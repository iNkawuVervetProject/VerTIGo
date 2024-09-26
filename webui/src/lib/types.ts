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

export interface CameraParameter {
	framerate: number;

	fileBitrate: number;
	fileSpeedPreset: string;

	awbMode: string;
	autoFocusMode: string;
	autoFocusRange: string;

	lensPosition: number;

	path: string;

	toServer(): any;
}

export function cameraParameterToServer(v: Partial<CameraParameter>): any {
	return {
		Framerate: v.framerate,
		FileBitrate: v.fileBitrate,
		FileSpeedPreset: v.fileSpeedPreset,
		AwbMode: v.awbMode,
		AutoFocusMode: v.autoFocusMode,
		AfRange: v.autoFocusRange,
		LensPosition: v.lensPosition
		// Path / RtspServerPath is omitted.
	};
}

export function cameraParameterFromServer(v: any): CameraParameter {
	let path;

	if (v.RtspServerPath !== undefined) {
		path = new URL(v.RtspServerPath).pathname;
	} else {
		path = v.Path ?? '/camera-live';
	}

	const res = {
		framerate: v.Framerate ?? 30.0,
		fileBitrate: v.FileBitrate ?? 2500,
		fileSpeedPreset: v.FileSpeedPreset ?? 'fast',
		awbMode: v.AwbMode ?? 'awb-auto',
		autoFocusMode: v.AutoFocusMode ?? 'automatic-auto-focus',
		autoFocusRange: v.AfRange ?? 'af-range-normal',
		lensPosition: v.LensPosition ?? 0.0,
		path: path,
		toServer: () => cameraParameterToServer(res)
	};
	return res;
}

export interface WindowParameter {
	color: string;
}

export function windowParameterFromServer(v: any) {
	return { color: v.color ?? '#000000' };
}
