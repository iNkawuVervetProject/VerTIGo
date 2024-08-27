import { dev } from '$app/environment';
import { env } from '$env/dynamic/private';
import { PUBLIC_NO_LOCAL_DEV_ENDPOINT } from '$env/static/public';
import type { Handle } from '@sveltejs/kit';

const BACKEND_HOST = env.BACKEND_HOST ?? 'localhost:5000';
const MEDIAMTX_HOST = env.MEDIAMTX_HOST ?? 'localhost:8889';
const DEBUG = (env.DEBUG ?? '0') != '0';

if (PUBLIC_NO_LOCAL_DEV_ENDPOINT != '0' || !dev) {
	console.log("Will redirect /psysw/api' to http://" + BACKEND_HOST);
}

async function proxyRequest(
	request: Request,
	oldOrigin: string,
	newHost: string,
	protocol: string = 'http://'
) {
	const newOrigin = protocol + newHost;
	const proxyRequest: Request = new Request(request.url.replace(oldOrigin, newOrigin), request);

	proxyRequest.headers.set('host', newHost);
	proxyRequest.headers.set('origin', newOrigin);
	proxyRequest.headers.set(
		'referer',
		request.headers.get('referer')?.replace(oldOrigin, newOrigin) || newOrigin + '/'
	);
	return await fetch(proxyRequest);
}

export const handle: Handle = async ({ event, resolve }) => {
	if (DEBUG) {
		console.log(event);
	}

	if (event.url.pathname.endsWith('whep')) {
		return await proxyRequest(event.request, event.url.origin, MEDIAMTX_HOST);
	}

	if (PUBLIC_NO_LOCAL_DEV_ENDPOINT === '0' && dev) {
		return await resolve(event);
	}

	if (event.url.pathname.startsWith('/psysw/api')) {
		return proxyRequest(event.request, event.url.origin + '/psysw/api', BACKEND_HOST);
	}

	return await resolve(event);
};
