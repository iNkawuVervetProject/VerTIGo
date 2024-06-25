import { participantService } from '$lib/server/participants';
import type { RequestHandler } from '@sveltejs/kit';

export const GET: RequestHandler = async ({}) => {
	return Response.json(participantService.participants);
};
