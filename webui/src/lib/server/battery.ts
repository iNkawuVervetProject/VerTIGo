import { dev } from '$app/environment';
import type { BatteryState } from '$lib/types';

import { PUBLIC_NO_LOCAL_DEV_ENDPOINT } from '$env/static/public';
import { env } from '$env/dynamic/private';
import { Socket } from 'net';

let state: BatteryState | undefined = undefined;
export function getBatteryState(): BatteryState | undefined {
	return state;
}

const FAKE_SERVICE: boolean = PUBLIC_NO_LOCAL_DEV_ENDPOINT == '0' && dev;
const NUT_HOSTNAME = env.NUT_HOSTNAME || 'localhost';

class NUTConnect {
	private constructor(private socket: Socket | undefined) {
		this.socket?.setEncoding('utf-8');
		this.socket?.on('close', () => {
			this.socket = undefined;
		});
	}

	private async send(command: string): Promise<string> {
		if (this.socket == undefined) {
			throw new Error('closed connection');
		}

		return new Promise((resolve, reject) => {
			let buffer: string = '';
			const onData = (chunk: string) => {
				buffer += chunk;
				const newLineIndex = buffer.indexOf('\n');
				if (newLineIndex !== -1) {
					const response = buffer.slice(0, newLineIndex);
					this.socket?.removeListener('data', onData);
					resolve(response.trim());
				}
			};

			this.socket?.on('data', onData);
			this.socket?.write(command.trim() + '\n', (err) => {
				if (err) {
					reject(err);
				}
			});
		});
	}

	async getVariable(ups: string, variable: string): Promise<string> {
		const prefix = `VAR ${ups.trim()} ${variable.trim()}`;
		const response = await this.send('GET ' + prefix);
		if (response.startsWith(prefix + ' "') == false) {
			throw new Error(`could not ${prefix}: ${response}`);
		}
		return response.slice(prefix.length + 2, -1);
	}

	static async connect(hostname: string, port: number): Promise<NUTConnect> {
		return new Promise((resolve, reject) => {
			const socket = new Socket();
			socket.on('error', (err: Error) => {
				reject(err);
			});

			socket.on('connect', () => {
				socket.removeAllListeners();
				resolve(new NUTConnect(socket));
			});

			socket.connect(port, hostname);
		});
	}
}

let nut: NUTConnect | undefined = undefined;

async function readBatteryState(): Promise<void> {
	if (nut == undefined) {
		try {
			nut = await NUTConnect.connect(NUT_HOSTNAME, 3493);
		} catch (e) {
			console.error('could not connect to nut: ' + e);
			return;
		}
	}

	try {
		const charge = await nut.getVariable('openups', 'battery.charge');

		const status: string = await nut.getVariable('openups', 'ups.status');

		state = { level: parseInt(charge), onBattery: false, charging: false };
		if (status.includes('OB') || status.includes('LB')) {
			state.onBattery = true;
			state.charging = false;
		} else {
			state.onBattery = false;
			state.charging = status.includes('CHRG');
		}
	} catch (e) {
		if (e instanceof Error && e.message == 'closed connection') {
			nut = undefined;
		}
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
