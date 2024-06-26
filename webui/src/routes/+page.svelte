<script lang="ts">
	import Experiment from '$lib/experiment.svelte';
	import { catalog } from '$lib/session_state';
	import ParticipantInput from '$lib/participant_input.svelte';
</script>

<section class="card variant-ghost-primary w-full space-y-4 p-4">
	<div class="grid gap-8 md:grid-cols-2">
		<ParticipantInput />
	</div>
</section>

{#each Object.entries($catalog) as [key, experiment]}
	<Experiment {experiment} />
{/each}
