import type { RequestEvent } from '@sveltejs/kit';
import { afterAll, beforeAll, describe, expect, it } from 'vitest';
import { GET } from './+server';
import { clearFakeData, initFakeData } from '$lib/server/stub_state';

describe('api/battery', () => {
	const mockEvent = {} as unknown as RequestEvent<Partial<Record<string, string>>, string | null>;

	beforeAll(() => {
		initFakeData();
	});
	afterAll(() => {
		clearFakeData();
	});

	it('GET should return battery status', async () => {
		const resp = await GET(mockEvent);
		expect(resp.status).toEqual(200);
		expect(await resp.json()).toMatchObject({ charging: false, level: 90, onBattery: true });
	});
});
