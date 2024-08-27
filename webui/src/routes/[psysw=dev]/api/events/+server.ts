import type { RequestHandler } from '@sveltejs/kit';
import { state } from '$lib/server/stub_state';
import type { Unsubscriber } from 'svelte/store';

export const GET: RequestHandler = async ({}) => {
	let unsubscribe: Unsubscriber = () => {};
	const stream = new ReadableStream({
		start(controller) {
			unsubscribe = state.subscribeEvents((name: string, data: any) => {
				controller.enqueue(`event: ${name}\ndata: ${JSON.stringify(data)}\n\n`);
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
