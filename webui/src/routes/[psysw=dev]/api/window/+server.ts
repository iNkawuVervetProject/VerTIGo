import { closeWindow } from '$lib/server/stub_state';
import { error, json, type RequestHandler } from '@sveltejs/kit';

export const DELETE: RequestHandler = ({}) => {
	try {
		closeWindow();
	} catch (e: any) {
		error(404, e.toString());
	}

	return json(null);
};
