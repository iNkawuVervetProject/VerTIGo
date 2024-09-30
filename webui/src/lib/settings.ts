import type { Writable } from 'svelte/store';
import { cameraParameterFromServer, type CameraParameter, type WindowParameter } from './types';
import { persisted } from 'svelte-persisted-store';

import typesTI from './types-ti';
import settingsTI from './settings-ti';
import { createCheckers } from 'ts-interface-checker';

const checkers = createCheckers(typesTI, settingsTI);

export interface Settings {
	camera: CameraParameter;

	window: WindowParameter;
}

export const defaultSettings: Settings = {
	camera: cameraParameterFromServer({
		Framerate: 30.0,
		FileBitrate: 2500,
		FileSpeedPreset: 'fast',
		AwbMode: 'awb-auto',
		AutoFocusMode: 'automatic-auto-focus',
		AfRange: 'af-range-normal',
		LensPosition: 10.0
	}),
	window: { color: '#000000' }
};

export const settings: Writable<Settings> = persisted<Settings>('settings', defaultSettings, {
	beforeRead: (value: Settings): Settings => {
		if (checkers.Settings.test(value) === false) {
			return defaultSettings;
		}
		return value;
	}
});
