import { dev } from '$app/environment';
import { PUBLIC_NO_LOCAL_DEV_ENDPOINT } from '$env/static/public';
import type { Handle } from '@sveltejs/kit';

const BACKEND_PREFIX = '/psysw/api';

export const handle: Handle = async ({ event, resolve }) => {
	if (PUBLIC_NO_LOCAL_DEV_ENDPOINT === '0' && dev) {
		return await resolve(event);
	}
	o;
	if (event.url.pathname.startsWith(BACKEND_PREFIX) == true) {
		const response = await fetch(
			'http://localhost:5000' + event.url.pathname.slice(BACKEND_PREFIX.length)
		);

		return new Response(response.body, {
			headers: {
				'Content-Type': 'text/event-stream',
				'Cache-Control': 'no-cache'
			}
		});
	}

	return await resolve(event);
};
