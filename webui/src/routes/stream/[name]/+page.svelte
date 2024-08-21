<script lang="ts">
	import { page } from '$app/stores';
	import WhepPlayer from '$lib/whep_player.svelte';
</script>

<WhepPlayer url={$page.params.name} />
