<script lang="ts">
	import Experiment from '$lib/experiment.svelte';
	import { catalog, window, experiment, stream } from '$lib/application_state';
	import ParticipantInput from '$lib/participant_input.svelte';
	import SessionInput from '$lib/session_input.svelte';
	import { fade, slide } from 'svelte/transition';
	import WhepPlayer from '$lib/whep_player.svelte';
	import { flip } from 'svelte/animate';
	import { page } from '$app/stores';

	async function closeWindow(): Promise<void> {
		await fetch('/psysw/api/window', { method: 'DELETE' });
	}

	const fadeOptions = { delay: 0, duration: 300 };

	async function startCamera(): Promise<void> {
		await fetch('/api/camera', { method: 'POST', body: '{}' });
	}

	async function stopCamera(): Promise<void> {
		await fetch('/api/camera', { method: 'DELETE' });
	}

	$: streaming = $stream != '';

	async function toggleCamera(): Promise<void> {
		if (streaming) {
			await stopCamera();
		} else {
			await startCamera();
		}
	}
</script>

<div
	class="grid {streaming ? 'lg:grid-cols-2' : ''}"
	class:gap-8={$stream}
	class:gap-0={$stream.length === 0}
	style="transition: gap {fadeOptions.delay}ms;"
>
	<section
		class="card variant-ghost-primary order-2 flex w-full flex-col space-y-4 p-4 lg:order-1"
	>
		<div class="grid gap-4 {streaming ? 'xl:grid-cols-2' : 'lg:grid-cols-2'}">
			<ParticipantInput />
			<SessionInput />
		</div>
		<div class="hidden grow lg:flex" />
		<div class="flex flex-row flex-wrap gap-2">
			<button class="variant-filled-secondary btn" on:click={toggleCamera}>
				<div class="relative mr-2 flex h-4 w-4">
					{#if streaming}
						<i class="fa-solid fa-stop absolute left-0 top-0" transition:fade />
					{:else}
						<i class="fa-solid fa-camera absolute left-0 top-0" transition:fade />
					{/if}
				</div>
				<span class="min-w-24">{streaming ? 'Stop Camera' : 'Record'}</span>
			</button>
			{#if $window}
				<div transition:slide={fadeOptions}>
					<button
						class="variant-filled-primary btn"
						disabled={$experiment != ''}
						on:click={closeWindow}
					>
						<i class="fa-solid fa-desktop mr-4" />
						Show Desktop
					</button>
				</div>
			{/if}
		</div>
	</section>
	{#if $stream}
		<div class="order-1 lg:order-2" transition:slide={fadeOptions}>
			<WhepPlayer url={$page.url.origin + $stream} />
		</div>
	{/if}
</div>

{#each Object.entries($catalog) as [key, experiment] (key)}
	<div animate:flip transition:slide|global={fadeOptions}>
		<Experiment {experiment} />
	</div>
{/each}

<style>
</style>
