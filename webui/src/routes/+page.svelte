<script lang="ts">
	import { messages, catalog, experiment, window } from '$lib/session_state';

	async function startExperiment(key: string) {
		await fetch('/psysw/api/experiment', {
			method: 'POST',
			headers: {
				'Content-Type': 'application/json'
			},
			body: JSON.stringify({ key: key, parameters: {} })
		});
	}
</script>

<h1>Welcome to SvelteKit</h1>
<p>Visit <a href="https://kit.svelte.dev">kit.svelte.dev</a> to read the documentation</p>

<h2>Messages</h2>
{#each $messages as m}
	<p>{m.msg} - {m.now}</p>
{/each}

<h2>Experiment</h2>
{#each Object.entries($catalog) as [key, exp]}
	<p>{key}, {JSON.stringify(exp)}</p>
	<button
		on:click={(e) => {
			startExperiment(key);
		}}
	>
		Start
	</button>
{/each}

<h2>State</h2>
<p>Current Experiment: "{$experiment}"</p>
<p>Window: {$window}</p>
