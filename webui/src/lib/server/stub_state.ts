import { Participant, type Catalog, type Parameters, type ParticipantByName } from '$lib/types';
import { get, writable, type Unsubscriber, type Writable } from 'svelte/store';

// A stub session
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

	private _participants: ParticipantByName = Object.assign(
		{},
		...[
			new Participant('asari', 123453),
			new Participant('turian', 1),
			new Participant('salarian', 42)
		].map((p: Participant) => ({ [p.name]: p }))
	);

	private window: Writable<boolean> = writable<boolean>(false);
	private experiment: Writable<string> = writable<string>('');
	private catalog: Writable<Catalog> = writable<Catalog>(this._catalog);
	private participants: Writable<ParticipantByName> = writable<ParticipantByName>(
		this._participants
	);
	private timeout?: ReturnType<typeof setTimeout> = undefined;

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

		unsubscribe.push(
			this.participants.subscribe((value: ParticipantByName) => {
				onEvent('participantsUpdate', value);
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
		const participant = parameters.participant;
		if (!(participant in this._participants)) {
			this._participants[participant] = new Participant(participant, 0);
		}
		this._participants[participant].nextSession = Math.max(
			this._participants[participant].nextSession,
			parameters.session + 1
		);
		this.participants.set(this._participants);
		//experiment finishes after 10s
		this.timeout = setTimeout(() => {
			this._stopExperiment();
		}, 10000);
	}

	public stopExperiment(): void {
		if (this.timeout === undefined) {
			throw new Error('no experiment started');
		}
		this._stopExperiment();
	}

	private _stopExperiment(): void {
		if (this.timeout !== undefined) {
			clearTimeout(this.timeout);
		}
		this.timeout = undefined;
		this.experiment.set('');
	}

	public closeWindow(): void {
		if (get(this.window) == false) {
			throw new Error('window is not opened');
		}
		this.window.set(false);
	}

	public getParticipants(): ParticipantByName {
		return this._participants;
	}
}

export const state: Session = new Session();
