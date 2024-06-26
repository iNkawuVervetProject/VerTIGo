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

<section class="card w-full">
	<div class="space-y-4 p-4"></div>
</section>
