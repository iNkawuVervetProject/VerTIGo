import { resources } from '$lib/server/stub_resources';
import { error, json, type RequestHandler } from '@sveltejs/kit';

export const GET: RequestHandler = ({ params }) => {
	if (resources.hasOwnProperty(params.id || '') == false) {
		error(404, 'Not found');
	}
	return json(resources[params.id || '']);
};
