import { dev } from '$app/environment';
import type { BatteryState } from '$lib/types';

import { NUTClient } from './nut';
import { PUBLIC_NO_LOCAL_DEV_ENDPOINT } from '$env/static/public';
import { env } from '$env/dynamic/private';

let state: BatteryState | undefined = undefined;
export function getBatteryState(): BatteryState | undefined {
	return state;
}

const FAKE_SERVICE: boolean = PUBLIC_NO_LOCAL_DEV_ENDPOINT == '0' && dev;
const NUT_HOSTNAME = env.NUT_HOSTNAME || 'localhost';

let nut: NUTClient | undefined = undefined;

async function readBatteryState(): Promise<void> {
	if (nut == undefined) {
		try {
			nut = await NUTClient.connect(NUT_HOSTNAME, 3493);
		} catch (e) {
			console.error('could not connect to nut: ' + e);
			return;
		}
	}

	try {
		const charge = await nut.getVariable('openups', 'battery.charge');

		const status = await nut.getVariable('openups', 'ups.status');

		console.log('battery: charge: ', charge, 'status: ', status);
	} catch (e) {
		console.error('could not get battery state: ' + e);
		state = undefined;
	}
}

if (FAKE_SERVICE) {
	state = {
		level: 90,
		onBattery: true,
		charging: false
	};

	let onChargerCount = 0;
	setInterval(() => {
		if (state?.onBattery) {
			state.level -= 1;
			if (state.level == 1) {
				state.onBattery = false;
				state.charging = true;
			}
		} else if (state?.charging) {
			state.level += 1;
			if (state.level == 98) {
				state.charging = false;
				onChargerCount = 0;
			}
		} else if (state != undefined) {
			if (onChargerCount++ == 10) {
				state.onBattery = true;
			}
		}
	}, 300);
} else {
	setInterval(async () => {
		await readBatteryState();
	}, 15000);
	await readBatteryState();
}
