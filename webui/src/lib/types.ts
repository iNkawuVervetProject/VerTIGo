export interface Catalog {
	[key: string]: Experiment;
}

export interface Parameters {
	participant: string;
	session: number;
	[key: string]: string | number;
}

export interface PsychopyError {
	title: string;
	details: string;
}
export interface Experiment {
	key: string;
	name: string;
	resources: { [key: string]: boolean };
	parameters: string[];
	errors: PsychopyError[];
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

export interface CameraResolution {
	Width: number;
	Height: number;
}

export interface CameraParameter {
	Framerate: number;

	FileBitrate: number;
	FileSpeedPreset: string;

	AwbMode: string;
	AutoFocusMode: string;
	AfRange: string;

	LensPosition: number;
}
