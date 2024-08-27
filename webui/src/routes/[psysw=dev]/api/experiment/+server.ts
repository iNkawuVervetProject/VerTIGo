import { runExperiment, stopExperiment } from '$lib/server/stub_state';
import type { RequestHandler } from '@sveltejs/kit';

export const DELETE: RequestHandler = ({}) => {
	try {
		stopExperiment();
	} catch (e) {
		return new Response('Internal Server Error', { status: 500 });
	}

	return new Response(null, { status: 200 });
};

export const POST: RequestHandler = async ({ request }) => {
	try {
		const data = await request.json();

		runExperiment(data.key, data.parameters);
	} catch (e) {
		return new Response('Internal Server Error', { status: 500 });
	}

	return new Response(null, { status: 200 });
};
