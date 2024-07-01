<script lang="ts">
	import type { BatteryState } from './types';

	export let state: Partial<BatteryState>;
</script>

<div class="flex flex-row items-center gap-2 p-4">
	<p>{state.level || '??'}%</p>
	<i
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
	/>
</div>
