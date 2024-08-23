<script lang="ts">
	import { page } from '$app/stores';
	import WhepPlayer from '$lib/whep_player.svelte';

	$: url = `${$page.url.origin}/${$page.params.name}`;
</script>

<WhepPlayer {url} />
