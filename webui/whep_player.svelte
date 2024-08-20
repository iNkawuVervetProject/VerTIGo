<script lang="ts">
	import { WHEPConnection } from '$lib/whep_connection';
	import { onDestroy, onMount } from 'svelte';

	export let url: string;

	let video: HTMLMediaElement;

	let connection: WHEPConnection | undefined;

	let mounted = false;

	let message: string = '';

	let timeout: any = undefined;

	function showError(err?: Error) {
		if (err === undefined) {
			message = '';
		} else {
			message = err.toString() + ', will retry in an instant';
		}
	}

	$: {
		const error = connection?.error;
		showError($error);

		if ($error !== undefined) {
			if (connection !== undefined) {
				connection = undefined;
			}

			scheduleReconnect();
		}
	}

	async function connect(url: string) {
		if ( timeout !== undefined ) {
			clearTimeout(timeout);
			timeout = undefined;
		}

		connection?.close();
		connection = undefined;

		try {
			connection = await WHEPConnection.connect(url);
		} catch (err: any) {
			showError(err);
			scheduleReconnect();
		}
	}

	function scheduleReconnect() {
		if (timeout !== undefined) {
			return;
		}
		timeout = setTimeout(()=>{
			timeout = undefined;
			connect(url);
		}
	}

	$: {
		const source = connection?.source;
		video.srcObject = $source || null;
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
