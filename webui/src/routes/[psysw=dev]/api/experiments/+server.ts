import { session } from '$lib/server/session';
import type { RequestHandler } from '@sveltejs/kit';

export const GET: RequestHandler = ({}) => {
	const data = JSON.stringify(session.experiments());
	return new Response(data);
};
