<script lang="ts">
	import { dev } from '$app/environment';
	import { page } from '$app/stores';
	import WhepPlayer from '$lib/whep_player.svelte';

	const origin = dev ? 'http://localhost:8889' : '/stream';
</script>

<WhepPlayer url={origin + '/' + $page.params.name} />
