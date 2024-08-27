import { battery } from '$lib/application_state';
import { error, json, type RequestHandler } from '@sveltejs/kit';
import { get } from 'svelte/store';

export const GET: RequestHandler = ({}) => {
	const value = get(battery);
	if (JSON.stringify(battery) == JSON.stringify({})) {
		return error(404, { message: 'Battery state unavailable' });
	}
	return json(value);
};
