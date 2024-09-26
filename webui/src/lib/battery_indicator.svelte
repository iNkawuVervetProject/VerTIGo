<script lang="ts">
	import type { MaybeBatteryState } from './application_state';

	export let state: MaybeBatteryState;
</script>

<div class="flex flex-row items-center gap-1">
	<i
		class="fa-solid min-w-6"
		class:fa-car-battery={state?.onBattery ?? false}
		class:fa-plug={!state?.charging ?? false}
		class:fa-plug-circle-bolt={state?.charging ?? false}
		class:fa-plug-circle-exclamation={state?.charging == undefined}
		class:text-warning-500={(state?.level ?? 10) < 10 && (state?.level ?? 10) > 5}
		class:text-error-500={state === null || state?.level <= 5}
	/>
	<p class="min-w-8 text-left">{state?.level ?? '??'}%</p>
</div>
