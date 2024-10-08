import { server, stream } from '$lib/application_state';
import { FAKE_BACKEND } from '$lib/env';
import { CAMERA_URL } from '$lib/server/env';
import { camera, startCamera, stopCamera } from '$lib/server/stub_state';
import type { CameraParameter } from '$lib/types';
import { error, json, type RequestHandler } from '@sveltejs/kit';
import { get } from 'svelte/store';

export const DELETE: RequestHandler = async ({ fetch }) => {
	if (FAKE_BACKEND) {
		try {
			stopCamera();
			return json(null);
		} catch (err: any) {
			error(404, err.toString());
		}
	} else {
		try {
			const resp = await fetch(CAMERA_URL, { method: 'DELETE' });
			if (resp.status !== 200) {
				return new Response(await resp.text(), { status: resp.status });
			}
			server.stream?.set('');
			return json(await resp.json());
		} catch (err) {
			error(502, `bad gateway: ${err}`);
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
			error(404, 'camera is not started');
		} else {
			return json(camera);
		}
	} else {
		try {
			const resp = await fetch(CAMERA_URL);
			if (resp.status !== 200) {
				return new Response(await resp.text(), { status: resp.status });
			}
			const params = (await resp.json()) as CameraParameter;
			server.stream?.set(streamPath(params));
			return json(params);
		} catch (err) {
			error(502, `bad gateway: ${err}`);
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
			error(400, err.toString());
		}
	} else {
		try {
			const resp = await fetch(CAMERA_URL, {
				method: 'POST',
				headers: {
					'Content-Type': 'application/json'
				},
				body: JSON.stringify(params)
			});
			if (resp.status !== 200) {
				// do not use error here.
				return new Response(await resp.text(), { status: resp.status });
			}
			params = (await resp.json()) as CameraParameter;
			server.stream?.set(streamPath(params));
			return json(params);
		} catch (err: any) {
			error(502, `bad gateway: ${err}`);
		}
	}
};
