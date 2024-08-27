import { readonly, writable, type Writable, type Readable, type Unsubscriber } from 'svelte/store';
import type { BatteryState, Catalog, Experiment, Participant, ParticipantByName } from './types';
import { browser } from '$app/environment';

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
		},
		subscribeToDiff: (onDiff: (diff: Dict) => void): Unsubscriber => {
			let current: Dict | undefined = undefined;
			return subscribe((value: Dict) => {
				if (current === undefined) {
					onDiff(value);
				} else {
					onDiff(dictDiff(current, value));
				}
				current = value;
			});
		}
	};
}

const _catalog = dictStore<Experiment, Catalog>({});
const _window: Writable<boolean> = writable<boolean>(false);
const _experiment: Writable<string> = writable<string>('');
const _participants = dictStore<Participant, ParticipantByName>({});
const _battery: Writable<Partial<BatteryState>> = writable<Partial<BatteryState>>({});

export const window: Readable<boolean> = readonly<boolean>(_window);
export const catalog: Readable<Catalog> = readonly<Catalog>(_catalog);
export const experiment: Readable<string> = readonly<string>(_experiment);
export const participants: Readable<ParticipantByName> = readonly<ParticipantByName>(_participants);
export const battery: Readable<Partial<BatteryState>> = readonly(_battery);

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
	},
	batteryUpdate: (event: MessageEvent): void => {
		const data = JSON.parse(event.data) as Partial<BatteryState>;
		_battery.set(data);
	}
};

const _eventSubscriptions = {
	catalogUpdate: _catalog.subscribeToDiff,
	experimentUpdate: _experiment.subscribe,
	windowUpdate: _window.subscribe,
	participantsUpdate: _participants.subscribeToDiff,
	batteryUpdate: _battery.subscribe
};

let _source: EventSource | undefined = undefined;

export function clearEventSource(): void {
	if (_source === undefined) {
		return;
	}
	console.log('stopping source of events from ', _source.url);
	for (const [type, listener] of Object.entries(_eventListeners)) {
		_source.removeEventListener(type, listener);
	}
	_source = undefined;
}

export function setEventSource(source: EventSource): void {
	clearEventSource();
	source.onmessage = console.log;
	source.onopen = () => {
		console.log('setting source of event from ', source.url);
		_source = source;
		for (const [type, listener] of Object.entries(_eventListeners)) {
			_source.addEventListener(type, listener);
		}
	};
}

export type EventSubscription = (type: string, data: any) => void;

export function subscribeToEvents(onEvent: EventSubscription): Unsubscriber {
	const unsubscribers: Unsubscriber[] = [];

	for (const [type, subscribe] of Object.entries(_eventSubscriptions)) {
		unsubscribers.push(
			subscribe((value: any) => {
				onEvent(type, value);
			})
		);
	}

	return () => {
		for (const unsubscribe of unsubscribers) {
			unsubscribe();
		}
	};
}

export const testing = {
	dictStore,
	dictDiff
};

export const server = {
	experiment: browser ? undefined : _experiment,
	window: browser ? undefined : _window,
	participants: browser ? undefined : _participants,
	catalog: browser ? undefined : _catalog,
	battery: browser ? undefined : _battery
};
