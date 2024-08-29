import { clearEventSource, server, setEventSource } from '$lib/application_state';
import { FAKE_BACKEND } from '$lib/env';
import { BACKEND_HOST, CAMERA_URL, DEBUG, MEDIAMTX_HOST } from '$lib/server/env';
import { readBatteryState } from '$lib/server/battery';
import { clearFakeData, initFakeData } from '$lib/server/stub_state';
import type { CameraParameter } from '$lib/types';
import { error, type Handle } from '@sveltejs/kit';
import EventSource from 'eventsource';

let _timeout: ReturnType<typeof setTimeout> | undefined = undefined;

function _connect() {
	if (_timeout !== undefined) {
		clearTimeout(_timeout);
		_timeout = undefined;
	}

	const eventURL = `http://${BACKEND_HOST}/events`;
	const source = new EventSource(eventURL);
	console.log('Reading events from ', eventURL);
	setEventSource(source);
	source.onerror = (evt: any) => {
		if (evt.message === '') {
			return;
		}
		console.log(`${eventURL} error: `, evt);
		clearEventSource();
		if (_timeout === undefined) {
			_timeout = setTimeout(() => {
				_timeout = undefined;
				_connect();
			}, 2000);
		}
	};
}

async function updateBattery(): Promise<void> {
	try {
		server.battery?.set(await readBatteryState());
	} catch (err) {
		console.error('could not read battery state: ' + err);
		server.battery?.set({});
	}
}

async function updateStream(): Promise<void> {
	try {
		const resp = await fetch(CAMERA_URL);
		if (resp.status !== 200) {
			server.stream?.set('');
		} else {
			const params = (await resp.json()) as Partial<CameraParameter>;
			if (params.RtspServerPath) {
				server.stream?.set(new URL(params.RtspServerPath).pathname || '');
			} else {
				server.stream?.set('');
			}
		}
	} catch (err) {
		console.error('could not read camera parameters: ' + err);
		server.stream?.set('');
	}
}

if (FAKE_BACKEND) {
	initFakeData();
} else {
	console.log("Will redirect /psysw/api' to http://" + BACKEND_HOST);
	_connect();
	updateBattery();
	updateStream();
	setInterval(updateBattery, 5000);
	setInterval(updateStream, 5000);
}

if (import.meta && import.meta.hot) {
	import.meta.hot.dispose(() => {
		clearFakeData();
		if (_timeout !== undefined) {
			clearTimeout(_timeout);
			clearEventSource();
		}
	});
}

async function proxyRequest(
	request: Request,
	oldOrigin: string,
	newHost: string,
	protocol: string = 'http://'
) {
	console.log(`redirecting ${request.url} to ${protocol}${newHost}`);
	const newOrigin = protocol + newHost;
	const proxyRequest: Request = new Request(request.url.replace(oldOrigin, newOrigin), request);

	proxyRequest.headers.set('host', newHost);
	proxyRequest.headers.set('origin', newOrigin);
	proxyRequest.headers.set(
		'referer',
		request.headers.get('referer')?.replace(oldOrigin, newOrigin) || newOrigin + '/'
	);
	try {
		return await fetch(proxyRequest);
	} catch (err) {
		error(502, `bad gateway: ${err}`);
	}
}

export const handle: Handle = async ({ event, resolve }) => {
	if (DEBUG) {
		console.log(event);
	}

	if (event.url.pathname.endsWith('whep')) {
		return await proxyRequest(event.request, event.url.origin, MEDIAMTX_HOST);
	}

	if (FAKE_BACKEND === true) {
		return await resolve(event);
	}

	if (event.url.pathname.startsWith('/psysw/api')) {
		return proxyRequest(event.request, event.url.origin + '/psysw/api', BACKEND_HOST);
	}

	return await resolve(event);
};
