<script lang="ts">
	import Experiment from '$lib/experiment.svelte';
	import { catalog, window, experiment } from '$lib/session_state';
	import ParticipantInput from '$lib/participant_input.svelte';
	import SessionInput from '$lib/session_input.svelte';
	import { slide } from 'svelte/transition';

	async function closeWindow(): Promise<void> {
		await fetch('/psysw/api/window', { method: 'DELETE' });
	}

	const fadeOptions = { delay: 250, duration: 300 };
</script>

<section class="card variant-ghost-primary w-full space-y-4 p-4">
	<div class="grid gap-8 md:grid-cols-2 lg:grid-cols-4">
		<ParticipantInput />
		<SessionInput />
	</div>
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
</section>

{#each Object.entries($catalog) as [key, experiment]}
	<Experiment {experiment} />
{/each}
