import { runExperiment, stopExperiment } from '$lib/server/stub_state';
import { error, json, type RequestHandler } from '@sveltejs/kit';

export const DELETE: RequestHandler = ({}) => {
	try {
		stopExperiment();
	} catch (e: any) {
		return error(400, e.toString());
	}

	return json(null, { status: 200 });
};

export const POST: RequestHandler = async ({ request }) => {
	try {
		const data = await request.json();

		runExperiment(data.key, data.parameters, data.window);
	} catch (e: any) {
		return error(400, e.toString());
	}

	return json(null);
};
