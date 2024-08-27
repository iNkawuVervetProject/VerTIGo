import { readonly, writable, type Writable, type Readable, type Unsubscriber } from 'svelte/store';
import type { BatteryState, Catalog, Experiment, Participant, ParticipantByName } from './types';

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

export type EventSubscription = (name: string, data: any) => void;

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

function subscribeToMapDiff<Value, Dict extends { [key: string]: Value }>(
	store: Readable<Dict>,
	onEvent: EventSubscription,
	name: string
): Unsubscriber {
	let current: Dict | undefined = undefined;

	return store.subscribe((value: Dict) => {
		if (current === undefined) {
			onEvent(name, current);
		} else {
			onEvent(name, dictDiff(current, value));
		}
		current = value;
	});
}

export class VertigoState {
	protected _catalog = dictStore<Experiment, Catalog>({});
	protected _window: Writable<boolean> = writable<boolean>(false);
	protected _experiment: Writable<string> = writable<string>('');
	protected _participants = dictStore<Participant, ParticipantByName>({});
	protected _camera: Writable<boolean> = writable<boolean>(false);
	protected _battery: Writable<Partial<BatteryState>> = writable<Partial<BatteryState>>({});

	public window: Readable<boolean> = readonly<boolean>(this._window);
	public catalog: Readable<Catalog> = readonly<Catalog>(this._catalog);
	public experiment: Readable<string> = readonly<string>(this._experiment);
	public participants: Readable<ParticipantByName> = readonly<ParticipantByName>(
		this._participants
	);
	public messages: Writable<any[]> = writable<any[]>([]);
	public camera: Readable<boolean> = readonly<boolean>(this._camera);
	public battery: Readable<Partial<BatteryState>> = readonly<Partial<BatteryState>>(
		this._battery
	);

	public constructor(eventSource?: EventSource) {
		if (eventSource !== undefined) {
			this.setEventSource(eventSource);
		}
	}

	public setEventSource(eventSource: EventSource) {
		eventSource.onmessage = (event) => {
			const data = JSON.parse(event.data);
			this.messages.update((arr: any[]) => arr.concat(data));
		};
		eventSource.addEventListener('windowUpdate', (event) => {
			const data = JSON.parse(event.data);
			this._window.set(data);
		});
		eventSource.addEventListener('experimentUpdate', (event) => {
			this._experiment.set(JSON.parse(event.data));
		});
		eventSource.addEventListener('catalogUpdate', (event) => {
			const updates = JSON.parse(event.data) as Catalog;
			this._catalog.mergeDiffs(updates);
		});
		eventSource.addEventListener('participantsUpdate', (event) => {
			const updates = JSON.parse(event.data) as ParticipantByName;
			this._participants.mergeDiffs(updates);
		});
		eventSource.addEventListener('cameraUpdate', (event) => {
			const data = JSON.parse(event.data) as boolean;
			this._camera.set(data);
		});
		eventSource.addEventListener('batteryUpdate', (event) => {
			const data = JSON.parse(event.data) as Partial<BatteryState>;
			this._battery.set(data);
		});
	}

	public subcribeEvents(onEvent: EventSubscription): Unsubscriber {
		const unsubscribers: Unsubscriber[] = [];

		unsubscribers.push(subscribeToMapDiff(this.catalog, onEvent, 'catalogUpdate'));

		unsubscribers.push(
			this.experiment.subscribe((value: string) => {
				onEvent('experimentUpdate', value);
			})
		);

		unsubscribers.push(
			this.window.subscribe((value: boolean) => {
				onEvent('windowUpdate', value);
			})
		);

		unsubscribers.push(subscribeToMapDiff(this.participants, onEvent, 'participantsUpdate'));

		unsubscribers.push(
			this.camera.subscribe((value: boolean) => {
				onEvent('cameraUpdate', value);
			})
		);

		unsubscribers.push(
			this.battery.subscribe((value: Partial<BatteryState>) => {
				onEvent('batteryUpdate', value);
			})
		);

		return () => {
			unsubscribers.forEach((unsubscribe: Unsubscriber) => {
				unsubscribe();
			});
		};
	}
}

export const testing = {
	dictStore,
	dictDiff
};
