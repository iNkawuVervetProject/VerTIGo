<script lang="ts">
	import { WHEPConnection } from '$lib/whep_connection';
	import { onDestroy } from 'svelte';

	export let url: string;

	let message: string = '';

	let video: HTMLMediaElement;

	let connection: WHEPConnection | undefined;

	$: {
		if (url.length == 0) {
			connection?.close();
			connection = undefined;
			video.srcObject = null;
		} else {
			connection?.close();
			connection = undefined;

			WHEPConnection.connect(
				url,
				(evt: RTCTrackEvent) => {
					message = '';
					video.srcObject = evt.streams[0];
				},
				(err: Error) => {
					message = err.toString();
				},
				2000
			)
				.then((c) => {
					connection = c;
				})
				.catch((err: Error) => {
					message = err.toString();
				});
		}
	}

	onDestroy(() => {
		connection?.close();
	});
</script>

<video bind:this={video} />
{#if message.length > 0}
	<p>{message}</p>
{/if}
