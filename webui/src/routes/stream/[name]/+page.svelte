<script lang="ts">
	import { page } from '$app/stores';
	import WhepPlayer from '$lib/whep_player.svelte';

	$: url = `${$page.url.protocol}://${$page.url.hostname}:8889/${$page.params.name}`;
</script>

<WhepPlayer {url} />
