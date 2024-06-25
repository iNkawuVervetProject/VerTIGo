import type { Catalog, Parameters } from '$lib/types';
import { get, writable, type Unsubscriber, type Writable } from 'svelte/store';

class Session {
	private _catalog: Catalog = {
		'valid.psyexp': {
			key: 'valid.psyexp',
			resources: { 'somepic.png': true },
			parameters: ['participant', 'session']
		},
		'unvalid.psyexp': {
			key: 'unvalid.psyexp',
			resources: { 'missing.png': false, 'missing_again.png': false },
			parameters: ['participant', 'session']
		},
		'more.psyexp': {
			key: 'more.psyexp',
			resources: { 'somepic.png': true },
			parameters: ['participant', 'session', 'age']
		}
	};
	private window: Writable<boolean> = writable<boolean>(false);
	private experiment: Writable<string> = writable<string>('');
	private catalog: Writable<Catalog> = writable<Catalog>(this._catalog);

	private timeout: number = -1;
	public experiments(): Catalog {
		return this._catalog;
	}

	public subscribeEvents(onEvent: (name: string, data: any) => void): Unsubscriber {
		const unsubscribe: Unsubscriber[] = [];

		unsubscribe.push(
			this.catalog.subscribe((value: Catalog) => {
				onEvent('catalogUpdate', value);
			})
		);

		unsubscribe.push(
			this.experiment.subscribe((value: string) => {
				onEvent('experimentUpdate', value);
			})
		);

		unsubscribe.push(
			this.window.subscribe((value: boolean) => {
				onEvent('windowUpdate', value);
			})
		);

		return () => {
			unsubscribe.forEach((u: Unsubscriber) => {
				u();
			});
		};
	}

	public runExperiment(key: string, parameters: Parameters): void {
		let exp = this._catalog[key];
		if (exp === undefined) {
			throw new Error(`unknown experiment '${key}'`);
		}
		let currExperiment = get(this.experiment);
		if (currExperiment !== '') {
			throw new Error(`experiment '${currExperiment}' is already running`);
		}

		this.window.set(true);
		this.experiment.set(key);

		//experiment finishes after 10s
		this.timeout = setTimeout(() => {
			this._stopExperiment();
		}, 10000);
	}

	public stopExperiment(): void {
		if (this.timeout == -1) {
			throw new Error('no experiment started');
		}
		this._stopExperiment();
	}

	private _stopExperiment(): void {
		this.timeout = -1;
		this.experiment.set('');
	}

	public closeWindow(): void {
		if (get(this.window) == false) {
			throw new Error('window is not opened');
		}
		this.window.set(false);
	}
}

export const session: Session = new Session();
