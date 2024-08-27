import {
	Participant,
	type BatteryState,
	type Catalog,
	type Parameters,
	type ParticipantByName
} from '$lib/types';
import { VertigoState } from '$lib/vertigo_state.ts~';
import { get } from 'svelte/store';

// A stub session
class VertigoSession extends VertigoState {
	private _catalogData: Catalog = {
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

	private _participantsData: ParticipantByName = Object.assign(
		{},
		...[
			new Participant('asari', 123453),
			new Participant('turian', 1),
			new Participant('salarian', 42)
		].map((p: Participant) => ({ [p.name]: p }))
	);

	private timeout?: ReturnType<typeof setTimeout> = undefined;

	private _batteryData: BatteryState = {
		level: 90,
		onBattery: true,
		charging: false
	};

	private _onChargerCount: number = 0;

	public constructor() {
		super();

		this._catalog.set(this._catalogData);
		this._participants.set(this._participantsData);
		setInterval(() => {
			this._updateBatteryData();
		}, 300);
	}

	private _updateBatteryData(): void {
		if (this._batteryData?.onBattery) {
			this._batteryData.level -= 1;
			if (this._batteryData.level == 1) {
				this._batteryData.onBattery = false;
				this._batteryData.charging = true;
			}
		} else if (this._batteryData?.charging) {
			this._batteryData.level += 1;
			if (this._batteryData.level == 98) {
				this._batteryData.charging = false;
				this._onChargerCount = 0;
			}
		} else if (this._batteryData != undefined) {
			if (this._onChargerCount++ == 10) {
				this._batteryData.onBattery = true;
			}
		}
		this._battery.set(this._batteryData);
	}

	public experiments(): Catalog {
		return this._catalogData;
	}

	public runExperiment(key: string, parameters: Parameters): void {
		let exp = this._catalogData[key];
		if (exp === undefined) {
			throw new Error(`unknown experiment '${key}'`);
		}
		let currExperiment = get(this._experiment);
		if (currExperiment !== '') {
			throw new Error(`experiment '${currExperiment}' is already running`);
		}

		this._window.set(true);
		this._experiment.set(key);
		const participant = parameters.participant;
		if (!(participant in this._participantsData)) {
			this._participantsData[participant] = new Participant(participant, 0);
		}
		this._participantsData[participant].nextSession = Math.max(
			this._participantsData[participant].nextSession,
			parameters.session + 1
		);
		this._participants.set(this._participantsData);
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
		this._experiment.set('');
	}

	public closeWindow(): void {
		if (get(this.window) == false) {
			throw new Error('window is not opened');
		}
		this._window.set(false);
	}
}

export const stub_state: VertigoSession = new VertigoSession();
