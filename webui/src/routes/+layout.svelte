<script lang="ts">
	import '../app.postcss';
	import '@fortawesome/fontawesome-free/css/all.min.css';

	import { AppShell, AppBar, LightSwitch } from '@skeletonlabs/skeleton';

	import { setEventSource, clearEventSource, battery } from '$lib/application_state';
	import { initializeStores, Modal, Toast, getToastStore } from '@skeletonlabs/skeleton';
	import { onMount } from 'svelte';
	import BatteryIndicator from '$lib/battery_indicator.svelte';

	initializeStores();

	let timeout: ReturnType<typeof setInterval> | undefined = undefined;

	function connect() {
		if (timeout !== undefined) {
			clearTimeout(timeout);
			timeout = undefined;
		}
		console.log('connect to /api/events');
		const source = new EventSource('api/events');
		setEventSource(source);
		source.onerror = () => {
			console.log('/api/events: got error while reading events');
			clearEventSource();
			if (timeout === undefined) {
				timeout = setTimeout(() => {
					timeout = undefined;
					connect();
				}, 2000);
			}
		};
	}

	onMount(() => {
		connect();

		return () => {
			if (timeout !== undefined) {
				clearTimeout(timeout);
				timeout = undefined;
			}
			clearEventSource();
		};
	});

	const toasts = getToastStore();

	let previous: number | undefined = 100;
	let alertToast: string | undefined = undefined;
	let warningToast: string | undefined = undefined;

	function onNewBatteryValue(level: number | undefined, charging: boolean): void {
		if (level == undefined) {
			if (previous != undefined) {
				if (alertToast != undefined) {
					toasts.close(alertToast);
				}

				alertToast = toasts.trigger({
					message: 'Could not read battery status',
					autohide: false,
					background: 'variant-filled-error',
					hideDismiss: true,
					classes: 'w-96 md:w-[32rem]'
				});
			}

			previous = undefined;
			return;
		}

		if ((previous ?? 11) > 10 && level <= 10) {
			warningToast = toasts.trigger({
				message: 'Battery level is low, system will turnoff soon unless plugged.',
				autohide: false,
				background: 'variant-filled-warning',
				classes: 'w-96 md:w-[32rem]'
			});
		}

		if ((previous ?? 6) > 5 && level <= 5) {
			if (alertToast != undefined) {
				toasts.close(alertToast);
			}
			alertToast = toasts.trigger({
				message: 'Battery level is critically low, system will turnoff now.',
				autohide: false,
				background: 'variant-filled-error',
				hideDismiss: true,
				classes: 'w-96 md:w-[32rem]'
			});
		}

		previous = level;
		if ((charging || level > 5) && alertToast != undefined) {
			toasts.close(alertToast);
			alertToast = undefined;
		}

		if ((charging || level > 10) && warningToast != undefined) {
			toasts.close(warningToast);
			warningToast = undefined;
		}
	}

	$: onNewBatteryValue($battery.level, $battery.charging ?? false);
</script>

<Modal />
<Toast />
<!-- App Shell -->
<AppShell>
	<svelte:fragment slot="header">
		<!-- App Bar -->
		<AppBar>
			<svelte:fragment slot="lead">
				<strong class="text-xl uppercase">VerTIGo</strong>
			</svelte:fragment>
			<svelte:fragment slot="trail">
				<a
					class="variant-ghost-surface btn btn-sm"
					href="https://github.com/iNkawuVervetProject/VerTIGo"
					target="_blank"
					rel="noreferrer"
				>
					GitHub
				</a>
				<BatteryIndicator state={$battery} />
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
