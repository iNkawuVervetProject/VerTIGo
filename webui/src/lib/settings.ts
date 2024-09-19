import type { Writable } from 'svelte/store';
import type { CameraParameter } from './types';
import { persisted } from 'svelte-persisted-store';

export interface Settings {
	camera: CameraParameter;

	backgroundColor: string;
}

export const settings: Writable<Settings> = persisted<Settings>('settings', {
	camera: {
		Framerate: 30.0,
		FileBitrate: 2000,
		FileSpeedPreset: 'fast',
		AwbMode: 'awb-auto',
		AutoFocusMode: 'automatic-auto-focus',
		AfRange: 'af-range-normal',
		LensPosition: 0.0
	},
	backgroundColor: '#7f007f'
});
