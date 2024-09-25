import { server } from '$lib/application_state';
import { FAKE_BACKEND } from '$lib/env';
import { CAMERA_URL } from '$lib/server/env';
import { camera, startCamera, stopCamera } from '$lib/server/stub_state';
import {
	cameraParameterFromServer,
	cameraParameterToServer,
	type CameraParameter
} from '$lib/types';
import { error, json, type RequestHandler } from '@sveltejs/kit';
import { get, readable } from 'svelte/store';

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
			server.camera?.set(null);
			return json(await resp.json());
		} catch (err) {
			error(502, `bad gateway: ${err}`);
		}
	}
};

export const GET: RequestHandler = async ({ fetch }) => {
	if (FAKE_BACKEND) {
		if (get(server.camera || readable(null)) === null) {
			error(404, 'Error: camera is not running');
		}

		return json(cameraParameterFromServer(camera));
	} else {
		try {
			const resp = await fetch(CAMERA_URL);
			if (resp.status !== 200) {
				return new Response(await resp.text(), { status: resp.status });
			}
			const params = cameraParameterFromServer(await resp.json());
			server.camera?.set(params);
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
			return json(startCamera(cameraParameterToServer(params)));
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
				body: JSON.stringify(cameraParameterToServer(params))
			});
			if (resp.status !== 200) {
				// do not use error here.
				return new Response(await resp.text(), { status: resp.status });
			}
			params = cameraParameterFromServer(await resp.json());
			server.camera?.set(params);
			return json(params);
		} catch (err: any) {
			error(502, `bad gateway: ${err}`);
		}
	}
};
