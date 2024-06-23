<script lang="ts">
	import { messages } from '$lib/session_state';
</script>

<h1>Welcome to SvelteKit</h1>
<p>Visit <a href="https://kit.svelte.dev">kit.svelte.dev</a> to read the documentation</p>

{#each $messages as m}
	<p>{m.msg} - {m.now}</p>
{/each}
