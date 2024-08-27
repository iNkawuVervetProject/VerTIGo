import type { RequestHandler } from '@sveltejs/kit';
import { stub_state } from '$lib/server/vertigo_state_stub.ts~';
import type { Unsubscriber } from 'svelte/store';
import { PUBLIC_NO_LOCAL_DEV_ENDPOINT } from '$env/static/public';
import { dev } from '$app/environment';
import { application_state } from '$lib/server/application_state';

const FAKE_SERVICE: boolean = PUBLIC_NO_LOCAL_DEV_ENDPOINT == '0' && dev;

const state = FAKE_SERVICE ? stub_state : application_state;

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
