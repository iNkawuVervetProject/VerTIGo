<script lang="ts">
	import { read_whep, type WHEPConnection } from '$lib/whep_connection';
	import { onDestroy, onMount } from 'svelte';
	import { readable } from 'svelte/store';

	export let url: string = '';

	let video: HTMLMediaElement;

	let connection: WHEPConnection | undefined;

	let mounted = false;

	let message: string = 'Connecting...';

	let timeout: any = undefined;

	let nTrials: number = 0;

	let delay: number = 0;

	function showError(err?: Error) {
		if (err === undefined) {
			message = '';
		} else {
			message = `${err.toString()}, will retry after ${Math.round(delay / 1000)}s`;
		}
	}

	function disconnect() {
		connection?.close();
		connection = undefined;
	}

	$: error = connection?.error || undefined;

	$: {
		if ($error !== undefined) {
			if (connection !== undefined) {
				connection.close(); // not required, but better be sure than sorry
				connection = undefined;
			}

			scheduleReconnect();
		}
		showError($error);
	}

	async function connect(url: string) {
		if (timeout !== undefined) {
			clearTimeout(timeout);
			timeout = undefined;
		}

		connection?.close();
		connection = undefined;

		try {
			connection = await read_whep(url);
			nTrials = 0;
		} catch (err: any) {
			nTrials += 1;
			scheduleReconnect();
			showError(err);
		}
	}

	function scheduleReconnect() {
		if (timeout !== undefined) {
			return;
		}
		delay = retryTimeout();

		timeout = setTimeout(() => {
			timeout = undefined;
			connect(url);
		}, delay);
	}

	function retryTimeout(): number {
		const exp = Math.min(Math.max(nTrials, 1), 5);
		return Math.round(1000 * (2 ** exp + (Math.random() * 2 - 1) / 20));
	}

	$: media = connection?.media || readable(undefined);
	$: {
		if (video !== undefined) {
			video.srcObject = $media || null;
		}
	}

	$: {
		if (mounted) {
			if (url != '') {
				connect(url);
			} else {
				disconnect();
				message = 'waiting for URL';
			}
		}
	}

	onMount(() => {
		mounted = true;
	});

	onDestroy(() => {
		connection?.close();
		if (timeout !== undefined) {
			clearTimeout(timeout);
			timeout = undefined;
		}
	});
</script>

<div
	class="align-center relative flex aspect-video min-w-full flex-row justify-center bg-surface-800/20 dark:bg-surface-200/20"
>
	{#if message.length > 0}
		<div
			class="absolute bottom-0 left-0 right-0 top-0 z-20 flex items-center justify-center bg-surface-800/50 p-8"
		>
			<p class="h-min grow-0 text-2xl">{message}</p>
		</div>
	{:else}
		<div class="absolute bottom-0 left-0 right-0 top-0 z-10 flex items-center justify-center">
			<p class="h-min grow-0 p-8 text-2xl">
				<i class="fa-solid fa-camera px-2" /> Connecting...
			</p>
		</div>
	{/if}
	<!-- svelte-ignore a11y-media-has-caption -->
	<video bind:this={video} autoplay="true" class="z-10 aspect-video min-w-full" />
</div>
