import { readonly, writable, type Writable } from 'svelte/store';
import type { BatteryState } from './types';
import { onMount } from 'svelte';

const _battery: Writable<Partial<BatteryState>> = writable<Partial<BatteryState>>({});

export const battery = readonly(_battery);

let started: boolean = false;

export function synchroniseBattery() {
	onMount(() => {
		if (started) {
			return;
		}
		started = true;
		setInterval(async () => {
			try {
				const data = (await fetch('/api/battery')).json() as BatteryState;
				_battery.set(data);
			} catch (e) {
				_battery.set({});
			}
		}, 10000);
	});
}
