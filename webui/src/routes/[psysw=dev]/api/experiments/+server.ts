import { catalog } from '$lib/server/stub_state';
import { json, type RequestHandler } from '@sveltejs/kit';

export const GET: RequestHandler = ({}) => {
	return json(catalog);
};
