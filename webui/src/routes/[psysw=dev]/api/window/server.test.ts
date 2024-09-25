import { closeWindow, openWindow } from '$lib/server/stub_state';
import { afterEach, assert, beforeEach, describe, expect, it, vi } from 'vitest';
import { DELETE } from './+server';
import { isHttpError, type RequestEvent } from '@sveltejs/kit';
import { window } from '$lib/application_state';
import { get } from 'svelte/store';

describe('psysw/api/window', () => {
	const mockEvent = {} as unknown as RequestEvent<Partial<Record<string, string>>, string | null>;
	afterEach(() => {
		vi.clearAllMocks();
	});

	describe('without a window', () => {
		beforeEach(() => {
			try {
				closeWindow();
			} catch (e) {}
		});

		it('DELETE should return 404', async () => {
			try {
				await DELETE(mockEvent);
			} catch (e: any) {
				expect(isHttpError(e)).toBeTruthy();
				expect(e.status).toEqual(404);
				expect(e.body.message).toEqual('Error: window is not opened');
				return;
			}
			assert.fail('should have received an exception');
		});
	});

	describe('with a window', () => {
		beforeEach(() => {
			try {
				openWindow({ color: '#f00' });
			} catch (e) {}
		});
		it('DELETE should close window and return 200', async () => {
			expect(get(window)).toMatchObject({ color: '#f00' });
			const resp = await DELETE(mockEvent);
			expect(resp.status).toEqual(200);
			expect(await resp.json()).toBeNull();
			expect(get(window)).toBeNull();
		});
	});
});
