import { afterEach, describe, expect, it } from 'vitest';
import { GET } from './+server';
import type { RequestEvent } from '@sveltejs/kit';

describe('api/events', () => {
	const mockEvent = {} as unknown as RequestEvent<Partial<Record<string, string>>, string | null>;
	it('should always return an event stream', async () => {
		const resp = await GET(mockEvent);
		expect(resp.status).toEqual(200);

		expect(resp.headers.get('Content-Type')).toEqual('text/event-stream');
		expect(resp.headers.get('Cache-Control')).toEqual('no-cache');

		const reader = resp.body?.getReader();
		const expected = ['catalog', 'experiment', 'window', 'participants', 'battery', 'camera'];
		for (const e of expected) {
			const data = await reader?.read();
			expect(data).not.toBeUndefined();
			expect(data?.done).toStrictEqual(false);
			expect(data?.value).toMatch(new RegExp(`^event: ${e}Update\ndata: .*\n\n$`));
		}
	});
});
