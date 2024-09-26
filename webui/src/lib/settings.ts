import type { Writable } from 'svelte/store';
import { cameraParameterFromServer, type CameraParameter, type WindowParameter } from './types';
import { persisted } from 'svelte-persisted-store';

export interface Settings {
	camera: CameraParameter;

	window: WindowParameter;
}

export const settings: Writable<Settings> = persisted<Settings>('settings', {
	camera: cameraParameterFromServer({
		Framerate: 30.0,
		FileBitrate: 2000,
		FileSpeedPreset: 'fast',
		AwbMode: 'awb-auto',
		AutoFocusMode: 'automatic-auto-focus',
		AfRange: 'af-range-normal',
		LensPosition: 0.0
	}),
	window: { color: '#7f007f' }
});
