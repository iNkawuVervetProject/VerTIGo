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

	function showError(err?: Error) {
		if (err === undefined) {
			message = '';
		} else {
			message = err.toString() + ', will retry in an instant';
		}
	}

	$: error = connection?.$error || undefined;

	$: {
		showError($error);
		if ($error !== undefined) {
			if (connection !== undefined) {
				connection.close(); // not required, but better be sure than sorry
				connection = undefined;
			}

			scheduleReconnect();
		}
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
		} catch (err: any) {
			showError(err);
			scheduleReconnect();
		}
	}

	function scheduleReconnect() {
		if (timeout !== undefined) {
			return;
		}

		timeout = setTimeout(() => {
			timeout = undefined;
			connect(url);
		}, retryTimeout());
	}

	function retryTimeout(): number {
		return 2000;
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

<video bind:this={video} />
{#if message.length > 0}
	<p>{message}</p>
{/if}
