import { get } from 'svelte/store';
import { describe, it, expect } from 'vitest';
import { testing } from './vertigo_state.ts~';
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
