import { readonly, writable, type Writable, type Readable } from 'svelte/store';
import type { Catalog, Experiment, Participant, ParticipantByName } from './types';
import { onMount } from 'svelte';

let subscribed = false;

// a custom store that incrementally merge a dict. If a key points to a null object it is
// removed. the backend can the incrementally sends diff to the frontend.
function dictStore<Value, Dict extends { [key: string]: Value }>(obj: Dict) {
	const { subscribe, set } = writable<Dict>(obj);

	return {
		subscribe,
		data: obj,
		set,
		mergeDiffs: (other: Dict): void => {
			obj = { ...obj, ...other } as Dict;
			for (const [key, value] of Object.entries(other)) {
				if (value == null) {
					delete obj[key];
				}
			}
			set(obj);
		}
	};
}

function dictDiff<Value, Dict extends { [key: string]: Value }>(a: Dict, b: Dict): Dict {
	const diff: any = {};

	for (const [key, value] of Object.entries(a)) {
		const bValue = b[key];
		if (bValue === undefined) {
			diff[key] = null;
			continue;
		}
		if (JSON.stringify(bValue) != JSON.stringify(value)) {
			diff[key] = bValue;
		}
	}

	const aKeys = Object.keys(a);
	for (const [key, value] of Object.entries(b)) {
		if (aKeys.includes(key) === false) {
			diff[key] = value;
		}
	}

	return diff;
}

const _catalog = dictStore<Experiment, Catalog>({});
const _window: Writable<boolean> = writable<boolean>(false);
const _experiment: Writable<string> = writable<string>('');
const _participants = dictStore<Participant, ParticipantByName>({});

export const window: Readable<boolean> = readonly<boolean>(_window);
export const catalog: Readable<Catalog> = readonly<Catalog>(_catalog);
export const experiment: Readable<string> = readonly<string>(_experiment);
export const participants: Readable<ParticipantByName> = readonly<ParticipantByName>(_participants);

const _eventListeners = {
	catalogUpdate: (event: MessageEvent): void => {
		const updates = JSON.parse(event.data) as Catalog;
		_catalog.mergeDiffs(updates);
	},
	windowUpdate: (event: MessageEvent): void => {
		const data = JSON.parse(event.data) as boolean;
		_window.set(data);
	},
	experimentUpdate: (event: MessageEvent): void => {
		const data = JSON.parse(event.data) as string;
		_experiment.set(data);
	},
	participantsUpdate: (event: MessageEvent): void => {
		const data = JSON.parse(event.data) as ParticipantByName;
		_participants.mergeDiffs(data);
	}
};

let _source: EventSource | undefined = undefined;

export function clearEventSource(): void {
	if (_source === undefined) {
		return;
	}
	for (const [type, listener] of Object.entries(_eventListeners)) {
		_source.removeEventListener(type, listener);
	}
	_source = undefined;
}

export function setEventSource(source: EventSource): void {
	clearEventSource();

	source.onerror = () => {
		clearEventSource();
	};
	for (const [type, listener] of Object.entries(_eventListeners)) {
		_source?.addEventListener(type, listener);
	}
}

export const testing = {
	dictStore,
	dictDiff
};
