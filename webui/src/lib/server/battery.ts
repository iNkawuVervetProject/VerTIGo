import type { BatteryState } from '$lib/types';
import { env } from '$env/dynamic/private';
import { Socket } from 'net';

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

export async function readBatteryState(): Promise<BatteryState | Error> {
	if (nut == undefined) {
		try {
			nut = await NUTConnect.connect(NUT_HOSTNAME, 3493);
		} catch (e) {
			return new Error('could not connect to nut: ' + e);
		}
	}

	try {
		const charge = await nut.getVariable('openups', 'battery.charge');

		const status: string = await nut.getVariable('openups', 'ups.status');

		const state = { level: parseInt(charge), onBattery: false, charging: false };
		if (status.includes('OB') || status.includes('LB')) {
			state.onBattery = true;
			state.charging = false;
		} else {
			state.onBattery = false;
			state.charging = status.includes('CHRG');
		}
		return state;
	} catch (e) {
		if (e instanceof Error && e.message == 'closed connection') {
			nut = undefined;
		}
		return new Error('could not get battery state: ' + e);
	}
}
