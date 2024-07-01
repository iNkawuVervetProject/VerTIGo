import type { BatteryState } from '$lib/types';
import type { LayoutLoad } from './$types';

export const load: LayoutLoad = async ({}) => {
	try {
		const state = await fetch('/api/battery').then((resp) => resp.json());
		return {
			battery: state as Partial<BatteryState>
		};
	} catch (e) {
		return {
			battery: {} as Partial<BatteryState>
		};
	}
};
