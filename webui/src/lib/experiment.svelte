<script lang="ts">
	import type { Experiment } from '$lib/types';
	import { parameters } from './parameters';
	import { experiment as currentExperiment } from './session_state';

	export let experiment: Experiment;

	async function start() {
		const params = Object.assign(
			{},
			...experiment.parameters.map((p: string) => ({
				[p]: $parameters[p]
			}))
		);
		await fetch('/psysw/api/experiment', {
			method: 'POST',
			body: JSON.stringify({ key: experiment.key, parameters: params })
		});
	}

	async function stop() {
		await fetch('/psysw/api/experiment', { method: 'DELETE' });
	}

	$: missingResources = Object.entries(experiment.resources)
		.filter(([_, valid]: [string, boolean]) => !valid)
		.map(([path, _]) => path);

	$: missingParameters = experiment.parameters?.filter(
		(p) => !(p in $parameters) || (p == 'participant' && $parameters.participant?.length === 0)
	);
	$: canRun =
		$currentExperiment === '' &&
		missingParameters.length === 0 &&
		missingResources.length === 0;
	$: isRunning = $currentExperiment === experiment.key;
</script>

<section
	class="card min-h-24 w-full space-y-4 p-8"
	class:variant-ghost-surface={!isRunning}
	class:variant-ghost-secondary={isRunning}
>
	<div class="flex">
		<h1 class="flex-auto text-2xl">{experiment.key}</h1>
		<button
			class="btn-icon justify-self-end"
			class:variant-filled-error={isRunning}
			class:variant-filled-success={!isRunning}
			disabled={!canRun && !isRunning}
			on:click={() => {
				if (isRunning) {
					stop();
				} else {
					start();
				}
			}}
		>
			<i class="fa-solid" class:fa-play={!isRunning} class:fa-stop={isRunning} />
		</button>
	</div>
	<dl class="list-dl">
		{#each missingResources as r}
			<div>
				<span class="variant-soft-error badge-icon p-4"
					><i class="fa-solid fa-file-circle-question" /></span
				>
				<span class="flex-auto">
					<dt class="font-bold">Missing resource</dt>
					<dd class="text-sm opacity-50">{r}</dd>
				</span>
			</div>
		{/each}
		{#each missingParameters as p}
			<div>
				<span class="variant-soft-warning badge-icon p-4"
					><i class="fa-solid fa-triangle-exclamation" /></span
				>
				<span class="flex-auto">
					<dt class="font-bold">Missing parameter</dt>
					<dd class="text-sm opacity-50">{p}</dd>
				</span>
			</div>
		{/each}
	</dl>
</section>
