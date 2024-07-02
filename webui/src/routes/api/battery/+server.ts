import { getBatteryState } from '$lib/server/battery';
import { error, json, type RequestHandler } from '@sveltejs/kit';

export const GET: RequestHandler = ({}) => {
	const battery = getBatteryState();
	if (battery == undefined) {
		return error(404, { message: 'Battery state unavailable' });
	}
	return json(battery);
};
