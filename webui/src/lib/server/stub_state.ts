import {
	cameraParameterFromServer,
	Participant,
	type BatteryState,
	type CameraParameter,
	type Catalog,
	type Parameters,
	type ParticipantByName,
	type WindowParameter
} from '$lib/types';
import { server, camera as _camera } from '$lib/application_state';
import { get, readable } from 'svelte/store';

export const catalog: Catalog = {
	'valid.psyexp': {
		key: 'valid.psyexp',
		name: 'valid',
		resources: { 'somepic.png': true },
		parameters: ['participant', 'session'],
		errors: []
	},
	'unvalid.psyexp': {
		key: 'unvalid.psyexp',
		name: 'unvalid',
		resources: { 'missing.png': false, 'missing_again.png': false },
		parameters: ['participant', 'session'],
		errors: []
	},
	'more.psyexp': {
		key: 'more.psyexp',
		name: 'something very long',
		resources: { 'somepic.png': true },
		parameters: ['participant', 'session', 'age'],
		errors: []
	},
	'with_error.0.1.psyexp': {
		key: 'with_error.0.1.psyexp',
		name: '',
		resources: {},
		parameters: [],
		errors: [
			{ title: 'invalid filename', details: "'with_error.0.1.psyexp' is an invalid filename" }
		]
	}
};

export const camera = {
	Framerate: 30,
	FileResolution: { Width: 1920, Height: 1080 },
	FileBitrate: 1500,
	FileSpeedPreset: 'ultrafast',

	StreamResolution: { Width: 854, Height: 480 },
	StreamBitrate: 400,
	RtspServerPath: 'http://localhost:8554/camera-live',

	AwbMode: 'awb-auto',
	AutoFocusMode: 'manual-focus',
	AfRange: 'af-range-normal',

	LensPosition: 0.0
};

export const participants: ParticipantByName = Object.assign(
	{},
	...[
		new Participant('asari', 123453),
		new Participant('turian', 1),
		new Participant('salarian', 42)
	].map((p: Participant) => ({ [p.name]: p }))
);

let _timeout: ReturnType<typeof setTimeout> | undefined = undefined;

export function openWindow(params: WindowParameter): void {
	server.window?.set(params);
}

let _onChargerCount = 0;
function _incrementStubBatteryState(state: BatteryState): BatteryState {
	if (state.onBattery) {
		state.level -= 1;
		if (state.level == 1) {
			state.onBattery = false;
			state.charging = true;
		}
	} else if (state?.charging) {
		state.level += 1;
		if (state.level == 98) {
			state.charging = false;
			_onChargerCount = 0;
		}
	} else if (state != undefined) {
		if (_onChargerCount++ == 10) {
			state.onBattery = true;
		}
	}
	return { level: state.level, charging: state.charging, onBattery: state.onBattery };
}

let _batteryInterval: ReturnType<typeof setInterval> | undefined;

export function initFakeData(): void {
	console.log('Will use stub backend data');
	server.experiment?.set('');
	server.catalog?.set(catalog);
	server.participants?.set(participants);

	let state: BatteryState = {
		level: 90,
		onBattery: true,
		charging: false
	};

	if (_batteryInterval === undefined) {
		_batteryInterval = setInterval(() => {
			state = _incrementStubBatteryState(state);
			server.battery?.set(state);
		}, 300);
	}
}

export function clearFakeData(): void {
	if (_batteryInterval !== undefined) {
		clearInterval(_batteryInterval);
		_batteryInterval = undefined;
	}
}

export function runExperiment(key: string, parameters: Parameters, window: WindowParameter): void {
	let exp = catalog[key];
	if (exp === undefined) {
		throw new Error(`unknown experiment '${key}'`);
	}
	let currExperiment = get(server.experiment || readable(''));
	if (currExperiment !== '') {
		throw new Error(`experiment '${currExperiment}' is already running`);
	}

	openWindow(window);
	server.experiment?.set(key);
	const participant = parameters.participant;
	if (!(participant in participants)) {
		participants[participant] = new Participant(participant, 0);
	}
	participants[participant].nextSession = Math.max(
		participants[participant].nextSession,
		parameters.session + 1
	);
	server.participants?.set(participants);
	//experiment finishes after 10s
	_timeout = setTimeout(() => {
		_stopExperiment();
	}, 10000);
}

export function stopExperiment(): void {
	if (_timeout === undefined) {
		throw new Error('no experiment started');
	}
	_stopExperiment();
}

function _stopExperiment(): void {
	if (_timeout !== undefined) {
		clearTimeout(_timeout);
	}
	_timeout = undefined;
	server.experiment?.set('');
}

export function closeWindow(): void {
	if (get(server.window || readable(null)) == null) {
		throw new Error('window is not opened');
	}
	server.window?.set(null);
}

export function startCamera(params: Partial<CameraParameter>): CameraParameter {
	if (get(_camera) !== null) {
		throw Error('camera is already started');
	}
	Object.assign(camera, {
		Framerate: params.framerate ?? 30,
		FileBitrate: params.fileBitrate ?? '1500',
		FileSpeedPreset: params.fileSpeedPreset ?? 'fast',

		AwbMode: params.awbMode ?? 'awb-auto',
		AutoFocusMode: params.autoFocusMode ?? 'automatic-auto-focus',
		AfRange: params.autoFocusRange ?? 'af-range-normal',
		LensPosition: params.lensPosition ?? 0.0
	});
	const res = cameraParameterFromServer(camera);
	server.camera?.set(res);
	return res;
}

export function stopCamera(): void {
	if (get(_camera) === null) {
		throw Error('camera is not started');
	}
	server.camera?.set(null);
}

export function getCamera(): any {
	if (get(_camera) === null) {
		throw Error('camera is not started');
	}
	return camera;
}
