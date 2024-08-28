<script lang="ts">
	import Experiment from '$lib/experiment.svelte';
	import { catalog, window, experiment, stream } from '$lib/application_state';
	import ParticipantInput from '$lib/participant_input.svelte';
	import SessionInput from '$lib/session_input.svelte';
	import { slide } from 'svelte/transition';
	import WhepPlayer from '$lib/whep_player.svelte';

	async function closeWindow(): Promise<void> {
		await fetch('/psysw/api/window', { method: 'DELETE' });
	}

	const fadeOptions = { delay: 250, duration: 300 };

	async function startCamera(): Promise<void> {
		await fetch('/api/camera', { method: 'POST', body: '{}' });
	}

	async function stopCamera(): Promise<void> {
		await fetch('/api/camera', { method: 'DELETE' });
	}
</script>

<div class="grid gap-8 lg:grid-cols-2">
	<section class="card variant-ghost-primary order-2 w-full space-y-4 p-4 lg:order-1">
		<div class="grid gap-4 lg:grid-cols-2">
			<ParticipantInput />
			<SessionInput />
			{#if $window}
				<div transition:slide={fadeOptions}>
					<button
						class="variant-filled-primary btn"
						disabled={$experiment != ''}
						on:click={closeWindow}
					>
						Show Desktop
					</button>
				</div>
			{/if}
			<div>
				{#if $stream == ''}
					<button class="variant-filled-secondary btn" on:click={startCamera}>
						<i class="fa-solid fa-camera mr-4" />
						Record
					</button>
				{:else}
					<button class="variant-filled-secondary btn" on:click={stopCamera}>
						<i class="fa-solid fa-stop mr-4" />
						Stop
					</button>
				{/if}
			</div>
		</div>
	</section>
	{#if $stream}
		<div class="order-1 lg:order-2" transition:slide={fadeOptions}>
			<WhepPlayer url={$stream} />
		</div>
	{/if}
</div>

{#each Object.entries($catalog) as [key, experiment]}
	<Experiment {experiment} />
{/each}
