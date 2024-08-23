<script lang="ts">
	import { read_whep, type WHEPConnection } from '$lib/whep_connection';
	import { onDestroy, onMount } from 'svelte';
	import { readable } from 'svelte/store';

	export let url: string;

	let video: HTMLMediaElement;

	let connection: WHEPConnection | undefined;

	let mounted = false;

	let message: string = 'none';

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
		return 3600 * Math.round(1000 * (2 ** exp + (Math.random() * 2 - 1) / 20));
	}

	$: media = connection?.media || readable(undefined);
	$: {
		if (video !== undefined) {
			video.srcObject = $media || null;
		}
	}

	$: {
		if (mounted) {
			connect(url);
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

<video bind:this={video} autoplay="true" />
{#if message.length > 0}
	<p>{message}</p>
{/if}
