import { participantService } from '$lib/server/participants';
import { error, json, type RequestHandler } from '@sveltejs/kit';

export const GET: RequestHandler = async ({ params }) => {
	if (!(params.name in participantService.participants)) {
		return error(404, `participant '{params.name}' not found`);
	}
	return json(participantService.participant(params.name));
};

export const POST: RequestHandler = async ({ params }) => {
	if (!(params.name in participantService.participants)) {
		return error(404, `participant '{params.name}' not found`);
	}
	participantService.incrementParticipantSession(params.name);
	return json(null);
};
