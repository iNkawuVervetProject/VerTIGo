import { describe, expect, it } from 'vitest';
import { GET } from './+server';
import type { RequestEvent } from '@sveltejs/kit';

describe('psysw/api/experiments', () => {
	const mockEvent = {} as unknown as RequestEvent<Partial<Record<string, string>>, string | null>;
	it('should return a 200', async () => {
		const resp = await GET(mockEvent);
		expect(resp.status).toEqual(200);
		expect(await resp.json()).toMatchObject({
			'valid.psyexp': {
				errors: []
			}
		});
	});
});
