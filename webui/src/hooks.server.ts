import { dev } from '$app/environment';
import { PUBLIC_NO_LOCAL_DEV_ENDPOINT } from '$env/static/public';
import type { Handle } from '@sveltejs/kit';

export const handle: Handle = async ({ event, resolve }) => {
	if (PUBLIC_NO_LOCAL_DEV_ENDPOINT === '0' && dev) {
		return await resolve(event);
	}

	// we intercept and proxy the text/event-stream
	if (event.url.pathname === '/psysw/api/events') {
		const proxy_url = 'http://localhost:5000/events';
		// keepalive is needed as otherwise the connection is closed and updates are not received
		// anymore. Will it leak? I dunno, lets see.
		const response = await fetch(proxy_url, { keepalive: true, cache: 'no-cache' });
		return new Response(response.body, {
			headers: {
				'Content-Type': 'text/event-stream',
				'Cache-Control': 'no-cache'
			}
		});
	}

	if (event.url.pathname.startsWith('/psysw/api')) {
		const request: Request = new Request(
			event.request.url.replace(event.url.origin + '/psysw/api', 'http://localhost:5000'),
			event.request
		);
		request.headers.set('host', 'localhost:5000');
		request.headers.set('origin', 'http://localhost:5000/');
		request.headers.set('referer', 'http://localhost:5000/');
		request.headers.set('content-type', 'application/json;charset=UTF-8');
		const response = await fetch(request);
		return response;
	}

	return await resolve(event);
};
