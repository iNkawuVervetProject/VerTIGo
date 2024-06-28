<script lang="ts">
	import '../app.postcss';
	import '@fortawesome/fontawesome-free/css/all.min.css';

	import { AppShell, AppBar, LightSwitch } from '@skeletonlabs/skeleton';

	import { synchronizeState } from '$lib/session_state';

	synchronizeState();
</script>

<!-- App Shell -->
<AppShell>
	<svelte:fragment slot="header">
		<!-- App Bar -->
		<AppBar>
			<svelte:fragment slot="lead">
				<strong class="text-xl uppercase">ZACI UI</strong>
			</svelte:fragment>
			<svelte:fragment slot="trail">
				<a
					class="variant-ghost-surface btn btn-sm"
					href="https://github.com/atuleu/zaci"
					target="_blank"
					rel="noreferrer"
				>
					GitHub
				</a>
				<LightSwitch />
			</svelte:fragment>
		</AppBar>
	</svelte:fragment>
	<!-- Page Route Content -->
	<div class="container mx-auto space-y-8 p-8">
		<slot />
	</div>
</AppShell>

<style></style>
