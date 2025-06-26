<script lang="ts">
	import { parameters } from './parameters';

	let inputValue: number | undefined = undefined;

	$: {
		$parameters.session = inputValue;
	}
</script>

<div class="flex items-center gap-2 xl:max-w-sm">
	<div class="input-group input-group-divider w-full grow grid-cols-[auto_1fr_auto]">
		<div class="input-group-shim">session</div>
		<input class="input" type="number" name="session" bind:value={inputValue} />
	</div>
</div>
