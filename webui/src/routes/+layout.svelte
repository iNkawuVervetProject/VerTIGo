<script lang="ts">
	import '../app.postcss';
	import '@fortawesome/fontawesome-free/css/all.min.css';

	import { AppShell, AppBar, LightSwitch } from '@skeletonlabs/skeleton';

	import { synchronizeState } from '$lib/session_state';
	import { initializeStores, Modal } from '@skeletonlabs/skeleton';
	import { invalidate } from '$app/navigation';
	import { onMount } from 'svelte';
	import type { LayoutData } from './$types';
	import { dev } from '$app/environment';
	import BatteryIndicator from '$lib/battery_indicator.svelte';

	initializeStores();
	synchronizeState();

	onMount(() => {
		const frequency = dev ? 2000 : 10000;
		const interval = setInterval(() => {
			invalidate('/');
			console.log('invalidating /');
		}, frequency);

		return () => clearInterval(interval);
	});

	export let data: LayoutData;
</script>

<Modal />
<!-- App Shell -->
<AppShell>
	<svelte:fragment slot="header">
		<!-- App Bar -->
		<AppBar>
			<svelte:fragment slot="lead">
				<strong class="text-xl uppercase">ZACI UI</strong>
			</svelte:fragment>
			<svelte:fragment slot="trail">
				<a
					class="variant-ghost-surface btn btn-sm"
					href="https://github.com/atuleu/zaci"
					target="_blank"
					rel="noreferrer"
				>
					GitHub
				</a>
				<BatteryIndicator state={data.battery} />
				<LightSwitch />
			</svelte:fragment>
		</AppBar>
	</svelte:fragment>
	<!-- Page Route Content -->
	<div class="container mx-auto space-y-8 p-8">
		<slot />
	</div>
</AppShell>

<style></style>
