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

export interface Settings {
	Camera: CameraParameter;

	BackgroundColor: string;
}

export const default_settings: Settings = {
	Camera: {
		Framerate: 30.0,
		FileBitrate: 2000,
		FileSpeedPreset: 'fast',
		AwbMode: 'awb-auto',
		AutoFocusMode: 'automatic-auto-focus',
		AfRange: 'af-range-normal',
		LensPosition: 0.0
	},
	BackgroundColor: '#000000'
};
