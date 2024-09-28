<script lang="ts">
	import { settings, type Settings } from '$lib/settings';
	import { getDrawerStore } from '@skeletonlabs/skeleton';
	import { camera, window } from './application_state';
	import { fade } from 'svelte/transition';
	import { derived, type Updater, type Writable } from 'svelte/store';

	$: modifiedWindow =
		$window !== null && JSON.stringify($window) !== JSON.stringify($settings.window);

	$: modifiedCamera =
		$camera !== null && JSON.stringify($camera) !== JSON.stringify($settings.camera);
	$: JSON.stringify($camera);

	const drawerStore = getDrawerStore();

	const autoWhiteBalancing = [
		{ value: 'awb-auto', label: 'Automatic' },
		{ value: 'awb-incandescent', label: 'Incandescent' },
		{ value: 'awb-tungsten', label: 'Tungsten' },
		{ value: 'awb-fluorescent', label: 'Fluorescent' },
		{ value: 'awb-indoor', label: 'Indoor' },
		{ value: 'awb-daylight', label: 'Daylight' },
		{ value: 'awb-cloudy', label: 'Cloudy' },
		{ value: 'awb-custom', label: 'Custom' }
	];

	const autoFocusMode = [
		{ value: 'manual-focus', label: 'Manual  Focus' },
		{ value: 'automatic-auto-focus', label: 'Automatic' },
		{ value: 'continuous-auto-focus', label: 'Continuous' }
	];

	const autoFocusRange = [
		{ value: 'af-range-normal', label: 'Normal' },
		{ value: 'af-range-macro', label: 'Macro' },
		{ value: 'af-range-full', label: 'Full Range' }
	];

	const positionMin = 1;
	const positionMax = 200;

	function centimeterToLensPosition(value: number): number {
		return 100.0 / Math.max(value, positionMin);
	}

	function lensPositionToCentimeter(value: number): number {
		return Math.round(Math.min(Math.max(100.0 / value, positionMin), positionMax));
	}

	const { subscribe } = derived(settings, (s: Settings) => {
		return lensPositionToCentimeter(s.camera.lensPosition);
	});

	const focalLength: Writable<number> = {
		subscribe: subscribe,
		set: (value: number): void => {
			settings.update((s: Settings) => {
				s.camera.lensPosition = centimeterToLensPosition(value);
				return s;
			});
		},
		update: (up: Updater<number>) => {
			settings.update((s: Settings) => {
				const res = up(lensPositionToCentimeter(s.camera.lensPosition));
				s.camera.lensPosition = centimeterToLensPosition(res);
				return s;
			});
		}
	};
</script>

<div class="container mx-auto p-4">
	<button type="button" class="variant-soft-surface btn-icon ml-4" on:click={drawerStore.close}>
		<i class="fa-solid fa-close" />
	</button>

	<div class="card m-4">
		<header class="card-header">
			<h2 class="text-lg"><i class="fa-solid fa-camera mr-2" />Experiment</h2>
		</header>
		<section class="grid gap-4 p-4 sm:grid-cols-2 lg:grid-cols-4">
			<label class="label">
				<span>Background Color</span>

				<div class="grid grid-cols-[auto_1fr] gap-2">
					<input class="input" type="color" bind:value={$settings.window.color} />
					<input
						class="input"
						type="text"
						bind:value={$settings.window.color}
						readonly
						tabindex="-1"
					/>
				</div>
			</label>
		</section>
		{#if modifiedWindow}
			<footer class="card-footer" transition:fade={{ duration: 200 }}>
				<aside
					class="variant-ghost-warning mx-auto flex flex-row items-start gap-4 p-4 sm:w-96"
				>
					<i class="fa-solid fa-warning my-0 text-4xl" />
					<p>
						Experiment's window is already opened. Changes won't take effect until you
						close it and reopen it again.
					</p>
				</aside>
			</footer>
		{/if}
	</div>

	<div class="card m-4">
		<header class="card-header">
			<h2 class="text-lg"><i class="fa-solid fa-camera mr-2" />Camera</h2>
		</header>
		<section class="grid gap-4 p-4 sm:grid-cols-2 lg:grid-cols-4">
			<label class="label">
				<span>Auto Focus Mode</span>
				<select class="select" bind:value={$settings.camera.autoFocusMode}>
					{#each autoFocusMode as { label, value } (value)}
						<option {value}>{label}</option>
					{/each}
				</select>
			</label>
			<label class="label">
				<span>Auto Focus Range</span>
				<select
					class="select"
					bind:value={$settings.camera.autoFocusRange}
					disabled={$settings.camera.autoFocusMode === 'manual-focus'}
				>
					{#each autoFocusRange as { label, value } (value)}
						<option {value}>{label}</option>
					{/each}
				</select>
			</label>
			<label class="label">
				<span>Lens Focus Position</span>
				<div class="input-group input-group-divider grid-cols-[1fr_auto]">
					<input
						class="input"
						type="number"
						min={positionMin}
						max={positionMax}
						bind:value={$focalLength}
						disabled={$settings.camera.autoFocusMode !== 'manual-focus'}
					/>
					<div class="input-group-shim">cm</div>
				</div>
			</label>
			<label class="label">
				<span> Auto White Balancing Mode</span>
				<select class="select" bind:value={$settings.camera.awbMode}>
					{#each autoWhiteBalancing as { label, value } (value)}
						<option {value}>{label}</option>
					{/each}
				</select>
			</label>
		</section>
		{#if modifiedCamera}
			<footer class="card-footer" transition:fade={{ duration: 200 }}>
				<aside
					class="variant-ghost-warning mx-auto flex flex-row items-start gap-4 p-4 sm:w-96"
				>
					<i class="fa-solid fa-warning my-0 text-4xl" />
					<p>
						Camera is already started. Changes won't take effect until you restart the
						recording
					</p>
				</aside>
			</footer>
		{/if}
	</div>
</div>
