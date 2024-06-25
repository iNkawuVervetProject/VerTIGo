<script lang="ts">
	import type { Experiment } from '$lib/types';
	import { parameters } from './parameters';

	export let experiment: Experiment;

	$: missingResources = Object.entries(experiment.resources)
		.filter(([_, valid]: [string, boolean]) => !valid)
		.map(([path, _]) => path);

	$: missingParameters = experiment.parameters?.filter(
		(p) => !(p in $parameters) || (p == 'participant' && $parameters.participant?.length === 0)
	);
	$: canRun = missingParameters.length === 0 && missingResources.length === 0;
</script>

<p>Title: {experiment.key}</p>
{#if canRun}
	<p>can be run</p>
{:else}
	<p>cannot be run:</p>
{/if}

<ul>
	{#each missingResources as r}
		<li>missing ressource '{r}'</li>
	{/each}
</ul>

<ul>
	{#each missingParameters as p}
		<li>missing parameter '{p}'</li>
	{/each}
</ul>
