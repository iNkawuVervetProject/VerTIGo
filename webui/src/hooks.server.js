let i = 0;

export async function handle({ event, resolve }) {
	if (event.url.pathname === '/lalala') {
		i += 1;
		console.log('touché', i);
		const response = await fetch('https://sse.dev/test');

		return new Response(response.body, {
			headers: {
				'Content-Type': 'text/event-stream',
				'Cache-Control': 'no-cache'
			}
		});
	}

	return await resolve(event);
}
