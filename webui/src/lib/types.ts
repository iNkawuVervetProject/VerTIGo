export interface Catalog {
	[key: string]: Experiment;
}

export interface Parameters {
	participant: string;
	session: number;
	[key: string]: string | number;
}

export interface Experiment {
	key: string;
	resources: { [key: string]: boolean };
	parameters: string[];
}

export class Participant {
	public constructor(
		public name: string,
		public nextSession: number = 1
	) {}
}

export interface ParticipantByName {
	[key: string]: Participant;
}

export interface BatteryState {
	level: number;
	onBattery: boolean;
	charging: boolean;
}
