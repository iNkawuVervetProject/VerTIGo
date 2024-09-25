import { describe, expect, it, beforeEach } from 'vitest';
import {
	closeWindow,
	getCamera,
	openWindow,
	runExperiment,
	startCamera,
	stopCamera,
	stopExperiment
} from './stub_state';
import { get } from 'svelte/store';
import { camera, experiment, participants, window } from '$lib/application_state';

describe('StubState', () => {
	beforeEach(() => {
		try {
			stopExperiment();
		} catch (e) {}

		try {
			closeWindow();
		} catch (e) {}
		try {
			stopCamera();
		} catch (e) {}
	});
	it('should be able to open and close a window', () => {
		expect(get(window)).toBeNull();
		openWindow({ color: '#000000' });
		expect(get(window)).toEqual({ color: '#000000' });
		closeWindow();
		expect(get(window)).toBeNull();
	});

	it('should be able to run an experiment', () => {
		expect(get(participants)).toEqual({});
		runExperiment(
			'valid.psyexp',
			{ participant: 'newPlayer', session: 1 },
			{ color: '#00ff00' }
		);
		expect(get(window)).toEqual({ color: '#00ff00' });
		expect(get(experiment)).toEqual('valid.psyexp');
		expect(Object.entries(get(participants))).toContainEqual([
			'newPlayer',
			{ name: 'newPlayer', nextSession: 2 }
		]);
		stopExperiment();
		expect(get(experiment)).toEqual('');
	});

	it('should be able to start the camera', () => {
		expect(get(camera)).toBeNull();
		startCamera({});
		expect(get(camera)).toMatchObject({
			framerate: 30,
			path: '/camera-live'
		});
		expect(getCamera()).toMatchObject({
			Framerate: 30,
			RtspServerPath: 'http://localhost:8554/camera-live'
		});
		stopCamera();
		expect(get(camera)).toBeNull();
	});

	it('should throw an exception when window is not opened', () => {
		expect(closeWindow).toThrowError('window is not opened');
	});

	it('should throw an exception when experiment is not running', () => {
		expect(stopExperiment).toThrowError('no experiment started');
	});

	it('should throw an exception when an experiment is already running', () => {
		runExperiment('valid.psyexp', { participant: 'already', session: 1 }, { color: '#000000' });
		expect(() => {
			runExperiment(
				'valid.psyexp',
				{ participant: 'already', session: 1 },
				{ color: '#000000' }
			);
		}).toThrowError("experiment 'valid.psyexp' is already running");
	});

	it('should throw an exception when camera is not running', () => {
		expect(stopCamera).toThrowError('camera is not started');
		expect(getCamera).toThrowError('camera is not started');
	});

	it('should throw an excpetion when camera is already started', () => {
		startCamera({});
		expect(() => startCamera({})).toThrowError('camera is already started');
	});
});
