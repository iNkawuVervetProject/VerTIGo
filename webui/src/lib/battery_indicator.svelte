<script lang="ts">
	import type { BatteryState } from './types';

	export let state: Partial<BatteryState>;
</script>

<div class="flex flex-row items-center gap-1">
	<!-- <i
		 class="fa-solid"
		 class:fa-battery-empty={state.level == undefined || state.level < 12.5}
		 class:fa-battery-quarter={state.level != undefined &&
									  state.level >= 12.5 &&
									  state.level < 37.5}
		 class:fa-battery-half={state.level != undefined &&
								   state.level >= 37.5 &&
								   state.level < 62.5}
		 class:fa-battery-three-quarters={state.level != undefined &&
											 state.level >= 62.5 &&
											 state.level < 87.5}
		 class:fa-battery-full={state.level != undefined && state.level >= 87.5}
		 class:text-error-500={state.level == undefined || state.level < 5}
		 class:text-warning-500={(state.level ?? 10) < 10}
		 /> -->
	<i
		class="fa-solid min-w-6"
		class:fa-car-battery={state.onBattery ?? false}
		class:fa-plug={!state.charging ?? false}
		class:fa-plug-circle-bolt={state.charging ?? false}
		class:fa-plug-circle-exclamation={state.charging == undefined}
		class:text-warning-500={(state.level ?? 10) < 10 && (state.level ?? 10) > 5}
		class:text-error-500={state.level == undefined || state.level <= 5}
	/>
	<p class="min-w-8 text-left">{state.level ?? '??'}%</p>
</div>
