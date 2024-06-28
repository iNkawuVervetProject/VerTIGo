import { dev } from '$app/environment';
import { PUBLIC_NO_LOCAL_DEV_ENDPOINT } from '$env/static/public';

import { exec as execClbk } from 'child_process';
import { promisify } from 'util';

const exec = promisify(execClbk);

interface BatteryState {
	level: number;
	onBattery: boolean;
	charging: boolean;
}

let state: BatteryState | undefined = undefined;
export function getBatteryState(): BatteryState | undefined {
	return state;
}

async function readBatteryState() {
	try {
		const { stdout } = await exec('upsc openups@localhost');
		if (state == undefined) {
			state = { level: -1, onBattery: false, charging: false };
		}

		for (const line of stdout.toString().split('\n')) {
			if (line.startsWith('battery.charge: ')) {
				state.level = parseInt(line.slice(16, -1));
				continue;
			}

			if (line.startsWith('ups.status: ')) {
				if (line.includes('OB') || line.includes('LB')) {
					state.onBattery = true;
					state.charging = false;
					continue;
				}
				state.onBattery = false;
				if (line.includes('CHRG')) {
					state.charging = true;
				} else {
					state.charging = false;
				}
			}
		}
	} catch (e) {
		state = undefined;
	}
}

if (PUBLIC_NO_LOCAL_DEV_ENDPOINT == '0' && dev) {
	state = {
		level: 90,
		onBattery: true,
		charging: false
	};

	let onChargerCount = 0;
	setInterval(() => {
		if (state?.onBattery) {
			state.level -= 1;
			if (state.level == 3) {
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
	}, 1000);
} else {
	setInterval(async () => {
		await readBatteryState();
	}, 15000);
}
