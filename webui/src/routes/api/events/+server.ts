import { subscribeToEvents } from '$lib/application_state';
import type { RequestHandler } from '@sveltejs/kit';
import type { Unsubscriber } from 'svelte/store';

export const GET: RequestHandler = async ({}) => {
	let unsubscribe: Unsubscriber = () => {};
	const stream = new ReadableStream({
		start(controller) {
			unsubscribe = subscribeToEvents((type: string, data: any) => {
				controller.enqueue(`event: ${type}\ndata: ${JSON.stringify(data)}\n\n`);
			});
		},
		cancel() {
			unsubscribe();
		}
	});

	return new Response(stream, {
		headers: {
			'Content-Type': 'text/event-stream',
			'Cache-Control': 'no-cache'
		}
	});
};
