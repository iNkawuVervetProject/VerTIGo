<script lang="ts">
	import type { Experiment } from '$lib/types';
	import { parameters } from './parameters';
	import { experiment as currentExperiment } from './session_state';

	export let experiment: Experiment;

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
	<h1 class="text-2xl">{experiment.key}</h1>
</section>
