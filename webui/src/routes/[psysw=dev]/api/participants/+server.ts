import { session } from '$lib/server/session';
import { json, type RequestHandler } from '@sveltejs/kit';

export const GET: RequestHandler = ({}) => {
	return json(session.getParticipants());
};
