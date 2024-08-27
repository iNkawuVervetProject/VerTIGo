import { closeWindow } from '$lib/server/stub_state';
import type { RequestHandler } from '@sveltejs/kit';

export const DELETE: RequestHandler = ({}) => {
	try {
		closeWindow();
	} catch (e) {
		return new Response('Internal Server Error', { status: 500 });
	}

	return new Response(null, { status: 200 });
};
