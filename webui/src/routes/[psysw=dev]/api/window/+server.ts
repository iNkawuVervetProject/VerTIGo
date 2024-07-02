import { session } from '$lib/server/session';
import type { RequestHandler } from '@sveltejs/kit';

export const DELETE: RequestHandler = ({}) => {
	try {
		session.closeWindow();
	} catch (e) {
		return new Response('Internal Server Error', { status: 500 });
	}

	return new Response(null, { status: 200 });
};
