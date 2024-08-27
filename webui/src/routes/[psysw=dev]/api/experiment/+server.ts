import { runExperiment, stopExperiment } from '$lib/server/stub_state';
import { error, type RequestHandler } from '@sveltejs/kit';

export const DELETE: RequestHandler = ({}) => {
	try {
		stopExperiment();
	} catch (e) {
		return error(500, `Internal Server Error: ${e}`);
	}

	return new Response(null, { status: 200 });
};

export const POST: RequestHandler = async ({ request }) => {
	try {
		const data = await request.json();

		runExperiment(data.key, data.parameters);
	} catch (e) {
		return error(500, `Internal Server Error: ${e}`);
	}

	return new Response(null, { status: 200 });
};
