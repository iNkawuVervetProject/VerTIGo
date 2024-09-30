import { get } from 'svelte/store';
import { describe, it, expect, vi, afterEach, beforeEach } from 'vitest';
import {
	battery,
	camera,
	catalog,
	clearEventSource,
	experiment,
	participants,
	server,
	setEventSource,
	testing,
	window
} from '$lib/application_state';
import type { BatteryState, Participant } from './types';
const { dictStore, dictDiff } = testing;

describe('dictStore', () => {
	it('should set it state to first value passed', () => {
		const store = dictStore({});
		expect(get(store)).toStrictEqual({});
	});

	it('should updates from diff', () => {
		const store = dictStore({ a: 'something' });
		expect(get(store)).toStrictEqual({ a: 'something' });
		store.mergeDiffs(JSON.parse('{ "a": "something else", "b": "another" }'));
		expect(get(store)).toStrictEqual({ a: 'something else', b: 'another' });
	});

	it('should deletes from diff', () => {
		const store = dictStore({ a: 'something' });
		expect(get(store)).toStrictEqual({ a: 'something' });
		store.mergeDiffs(JSON.parse('{"a":null}'));
		expect(get(store)).toStrictEqual({});
	});

	it('safe to delete unknows ', () => {
		const store = dictStore({ a: 'something' });
		expect(get(store)).toStrictEqual({ a: 'something' });
		store.mergeDiffs(JSON.parse('{"b":null}'));
		expect(get(store)).toStrictEqual({ a: 'something' });
	});
});

describe('dictDiff', () => {
	it('should report empty diffs', () => {
		expect(dictDiff({}, {})).toStrictEqual({});
		expect(dictDiff({ a: 1 }, { a: 1 })).toStrictEqual({});
	});

	it('should report new values', () => {
		expect(dictDiff({}, { a: 1 })).toStrictEqual({ a: 1 });
		expect(dictDiff({ a: 1 }, { a: 2 })).toStrictEqual({ a: 2 });
	});

	it('should report removed values', () => {
		expect(dictDiff({ a: 1 }, {})).toStrictEqual({ a: null });
	});
});

describe('ApplicationState', () => {
	it('should have correct initial state', () => {
		expect(get(window)).toBeNull();
		expect(get(camera)).toBeNull();
		expect(get(experiment)).toEqual('');
		expect(get(catalog)).toEqual({});
	});

	describe('can update from event listener', () => {
		const listeners: any = {};

		const source = {
			onmessage: vi.fn(),
			onopen: () => {},
			addEventListener: vi.fn((t: string, cbk: (event: MessageEvent) => void) => {
				listeners[t] = cbk;
			}),
			removeEventListener: vi.fn(),
			close: vi.fn()
		} as unknown as EventSource;

		const fire = (t: string, data: any) => {
			const cbk = listeners[t];
			if (cbk === undefined) {
				return;
			}
			cbk({ data: JSON.stringify(data) });
		};

		afterEach(() => {
			clearEventSource();
			expect(source.close).toHaveBeenCalled();
			vi.clearAllMocks();
		});

		beforeEach(() => {
			server.catalog?.set({});
			setEventSource(source);
			source.onopen();
			expect(source.addEventListener).toHaveBeenNthCalledWith(
				1,
				'catalogUpdate',
				expect.anything()
			);
		});

		it('should update catalog as incremental dict', () => {
			fire('catalogUpdate', {
				'valid.psyexp': {
					key: 'valid.psyexp',
					name: 'valid',
					resources: {},

					parameters: [],
					errors: []
				}
			});
			expect(get(catalog)).toMatchObject({ 'valid.psyexp': { key: 'valid.psyexp' } });
			fire('catalogUpdate', { 'valid.psyexp': null });
			expect(get(catalog)).toEqual({});
		});

		it('should update window as MaybeWindowParameter', () => {
			fire('windowUpdate', {});
			expect(get(window)).toMatchObject({ color: '#000000' });
			fire('windowUpdate', null);
			expect(get(window)).toBeNull();
		});

		it('should update experient as a string', () => {
			fire('experimentUpdate', 'foobar');
			expect(get(experiment)).toEqual('foobar');
		});

		it('should update participants as an incremental dict', () => {
			const incr = { foo: { name: 'foo', nextSession: 3 } as Participant };
			fire('participantsUpdate', incr);
			expect(get(participants)).toMatchObject(incr);
			fire('participantsUpdate', { foo: null });
			expect(get(participants)).toEqual({});
		});

		it('should update battery as a MaybeBatteryState', () => {
			const state: BatteryState = { level: 90, onBattery: false, charging: false };
			fire('batteryUpdate', state);
			expect(get(battery)).toEqual(state);
			fire('batteryUpdate', null);
			expect(get(battery)).toBeNull();
		});

		it('should update camera as a MaybeCameraParameter', () => {
			fire('cameraUpdate', {});
			expect(get(camera)).toMatchObject({ path: '/camera-live' });
			fire('cameraUpdate', null);
			expect(get(camera)).toBeNull();
		});
	});
});
