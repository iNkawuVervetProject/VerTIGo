import { dev } from '$app/environment';
import { env } from '$env/dynamic/private';
import { PUBLIC_NO_LOCAL_DEV_ENDPOINT } from '$env/static/public';
import { server, stream } from '$lib/application_state';
import { camera, startCamera, stopCamera } from '$lib/server/stub_state';
import type { CameraParameter } from '$lib/types';
import { error, json, type RequestHandler } from '@sveltejs/kit';
import { get } from 'svelte/store';

const FAKE_BACKEND = PUBLIC_NO_LOCAL_DEV_ENDPOINT === '0' && dev;
const CAMERA_HOST = env.CAMERA_HOST ?? 'localhost:5042';
const CAMERA_URL = `http://${CAMERA_HOST}/camera`;

export const DELETE: RequestHandler = async ({ fetch }) => {
	if (FAKE_BACKEND) {
		try {
			stopCamera();
			return json(null);
		} catch (err: any) {
			return error(404, err.toString());
		}
	} else {
		try {
			const resp = await fetch(CAMERA_URL, { method: 'DELETE' });
			if (resp.status === 200) {
				server.stream?.set('');
			}
			return json(await resp.json());
		} catch (err) {
			return error(502, `bad gateway: ${err}`);
		}
	}
};

function streamPath(params: Partial<CameraParameter>): string {
	if (params.RtspServerPath === undefined) {
		return 'camera-live';
	}
	return new URL(params.RtspServerPath).pathname;
}

export const GET: RequestHandler = async ({ fetch }) => {
	if (FAKE_BACKEND) {
		if (get(stream) == '') {
			return error(404, 'camera is not started');
		} else {
			return json(camera);
		}
	} else {
		try {
			const resp = await fetch(CAMERA_URL);
			if (resp.status !== 200) {
				return error(resp.status, await resp.text());
			}
			const params = (await resp.json()) as CameraParameter;
			server.stream?.set(streamPath(params));
			return json(params);
		} catch (err) {
			return error(502, `bad gateway: ${err}`);
		}
	}
};

export const POST: RequestHandler = async ({ fetch, request }) => {
	let params;
	try {
		params = (await request.json()) as Partial<CameraParameter>;
	} catch (err) {
		return error(400, `bad request ${err}`);
	}

	if (FAKE_BACKEND) {
		try {
			return json(startCamera(params));
		} catch (err: any) {
			return error(400, err.toString());
		}
	} else {
		try {
			const resp = await fetch(CAMERA_URL, {
				method: 'POST',
				body: JSON.stringify(params)
			});
			if (resp.status !== 200) {
				return error(resp.status, await resp.text());
			}
			params = (await resp.json()) as CameraParameter;
			server.stream?.set(streamPath(params));
			return json(params);
		} catch (err) {
			return error(502, `bad gateway: ${err}`);
		}
	}
};
