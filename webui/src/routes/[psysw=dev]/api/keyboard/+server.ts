import { emulateKeyPress } from '$lib/server/stub_state';
import { error, type RequestHandler } from '@sveltejs/kit';

export const POST: RequestHandler = async ({ request }) => {
	try {
		const data = await request.json();
		emulateKeyPress(data.key);
	} catch (e) {
		return error(500, `Internal Server Error: ${e}`);
	}

	return new Response(null, { status: 200 });
};
