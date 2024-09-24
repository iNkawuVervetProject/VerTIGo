import { describe, expect, it } from 'vitest';
import { cameraParameterFromServer, windowParameterFromServer } from './types';

describe('CameraParemeter', () => {
	const serverOutput =
		'{"Framerate":30,"FileResolution":{"Width":1920,"Height":1080},"FileBitrate":1500,"FileSpeedPreset":"fast","StreamResolution":{"Width":854,"Height":480},"StreamBitrate":400,"RtspServerPath":"rtsp://localhost:8554/camera-live","AwbMode":"awb-auto","AutoFocusMode":"automatic-auto-focus","AfRange":"af-range-normal","LensPosition":0.0}';

	const parsed = cameraParameterFromServer(JSON.parse(serverOutput));

	it('should parse server input', () => {
		expect(parsed).toEqual({
			framerate: 30,
			fileBitrate: 1500,
			fileSpeedPreset: 'fast',
			awbMode: 'awb-auto',
			autoFocusMode: 'automatic-auto-focus',
			autoFocusRange: 'af-range-normal',
			lensPosition: 0.0,
			path: '/camera-live',
			toServer: parsed.toServer
		});
	});

	it('should strip path', () => {
		const serialized = parsed.toServer();
		expect(serialized).not.toContain('path');
		expect(serialized).not.toContain('RtspServerPath');
	});
});

describe('WindowParameter', () => {
	const serverOutput =
		'{"size":[1280,800],"winType":"pyglet","allowStencil":false,"monitor":"testMonitor","color":"#7f7f00","colorSpace":"rgb","blendMode":"avg","useFBO":true,"units":"height","checkTiming":false,"fullscr":true,"screen":0}';

	it('should parse server input', () => {
		const parsed = windowParameterFromServer(JSON.parse(serverOutput));
		expect(parsed).toEqual({ color: '#7f7f00' });
	});
});
