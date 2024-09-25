import { afterEach, assert, beforeEach, describe, expect, it, vi } from 'vitest';
import { DELETE, POST } from './+server';
import { isHttpError, type RequestEvent } from '@sveltejs/kit';
import { runExperiment, stopExperiment } from '$lib/server/stub_state';

describe('psysw/api/experiment', () => {
	const mockEvent = {
		request: {
			json: vi.fn(async () => {
				return {
					key: 'valid.psyexp',
					parameters: { participant: 'tester', session: 3 },
					window: { color: '#000000' }
				};
			})
		}
	} as unknown as RequestEvent<Partial<Record<string, string>>, string | null>;
	afterEach(() => {
		vi.clearAllMocks();
	});

	describe('without any experiment running', () => {
		afterEach(() => {
			try {
				stopExperiment();
			} catch (e) {}
		});
		it('DELETE should return a 400 error', async () => {
			try {
				await DELETE(mockEvent);
			} catch (e: any) {
				expect(isHttpError(e)).toBeTruthy();
				expect(e.status).toEqual(400);
				expect(e.body.message).toEqual('Error: no experiment started');
				return;
			}
			assert.fail('an exception should be thrown');
		});

		it('POST should allow to start an experiment', async () => {
			const resp = await POST(mockEvent);
			expect(resp.status).toEqual(200);
			expect(await resp.json()).toBeNull();
		});
	});

	describe('with an experiment running', () => {
		beforeEach(() => {
			try {
				runExperiment(
					'valid.psyexp',
					{ participant: 'tester', session: 4 },
					{ color: '#000000' }
				);
			} catch (e) {}
		});
		afterEach(() => {
			try {
				stopExperiment();
			} catch (e) {}
		});
		it('DELETE should stop the experiment and return 200', async () => {
			const resp = await DELETE(mockEvent);
			expect(resp.status).toEqual(200);
			expect(await resp.json()).toBeNull();
		});

		it('POST should return a 400 error', async () => {
			try {
				await POST(mockEvent);
			} catch (e: any) {
				expect(isHttpError(e)).toBeTruthy();
				expect(e.status).toEqual(400);
				expect(e.body.message).toEqual(
					"Error: experiment 'valid.psyexp' is already running"
				);
				return;
			}
			assert.fail('should have thrown an exception');
		});
	});
});
