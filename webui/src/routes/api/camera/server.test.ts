import { afterEach, assert, beforeEach, describe, expect, it, vi } from 'vitest';
import { DELETE, GET, POST } from './+server';
import { isHttpError, type RequestEvent } from '@sveltejs/kit';
import { startCamera, stopCamera } from '$lib/server/stub_state';

type SKRequestEvent = RequestEvent<Partial<Record<string, string>>, string | null>;

describe('api/camera', () => {
	const mockEvent = {
		request: {
			json: vi.fn(async () => {
				return {};
			})
		}
	} as unknown as SKRequestEvent;

	afterEach(() => {
		vi.clearAllMocks();
	});

	describe('with no camera running', () => {
		beforeEach(() => {
			try {
				stopCamera();
			} catch (e) {}
		});

		it('GET should report the camera status', async () => {
			try {
				await GET(mockEvent);
			} catch (e: any) {
				expect(isHttpError(e)).toBeTruthy();
				expect(e.status).toEqual(404);
				expect(e.body.message).toEqual('Error: camera is not running');
				return;
			}
			assert.fail('should have thrown an HttpError');
		});

		it('POST should start the camera', async () => {
			const resp = await POST(mockEvent);
			expect(resp.status).toEqual(200);
			const data = await resp.json();
			expect(data).toMatchObject({ path: '/camera-live' });
		});

		it('DELETE should report that no camera is running', async () => {
			try {
				await DELETE(mockEvent);
			} catch (e: any) {
				expect(isHttpError(e)).toBeTruthy();
				expect(e.status).toEqual(404);
				expect(e.body.message).toEqual('Error: camera is not started');
				return;
			}
			assert.fail('should have thrown an exception');
		});
	});

	describe('with a camera running', () => {
		beforeEach(() => {
			try {
				startCamera({});
			} catch (e) {}
		});

		it('GET should report the camera status', async () => {
			const resp = await GET(mockEvent);
			expect(resp.status).toEqual(200);
			const data = await resp.json();
			expect(data).toMatchObject({
				path: '/camera-live'
			});
		});

		it('POST should return an error', async () => {
			try {
				await POST(mockEvent);
			} catch (e: any) {
				expect(isHttpError(e)).toBeTruthy();
				expect(e.status).toEqual(400);
				expect(e.body.message).toEqual('Error: camera is already started');
				return;
			}
			assert.fail('should have thrown an exception');
		});

		it('DELETE should return 200 with a null body', async () => {
			const resp = await DELETE(mockEvent);
			expect(resp.status).toEqual(200);
			expect(await resp.json()).toBeNull();
		});
	});
});
