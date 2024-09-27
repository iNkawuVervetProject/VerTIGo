<script lang="ts">
	import { settings } from '$lib/settings';
	import { window } from './application_state';

	const modifiedWindow = true;
	let color = '#000000';
	$: console.log($window);
</script>

<div class="card m-4">
	<header class="card-header">
		<h2 class="text-lg"><i class="fa-solid fa-camera mr-2" />Experiment</h2>
	</header>
	<section class="grid gap-4 p-4 sm:grid-cols-2 lg:grid-cols-4">
		<label class="label">
			<span>Background Color</span>

			<div class="grid grid-cols-[auto_1fr] gap-2">
				<input class="input" type="color" bind:value={color} />
				<input
					class="input"
					type="text"
					bind:value={$settings.window.color}
					readonly
					tabindex="-1"
				/>
			</div>
		</label>
		{#if modifiedWindow}
			<p>
				Experiment's window is already opened. Changes won't take effect until you close it
				and reopen it again.
			</p>
		{/if}
		<footer class="card-footer"></footer>
	</section>
</div>

<div class="card m-4">
	<header class="card-header">
		<h2 class="text-lg"><i class="fa-solid fa-camera mr-2" />Camera</h2>
	</header>
	<section class="grid gap-4 p-4 sm:grid-cols-2 lg:grid-cols-4">
		<label class="label">
			<span>Auto Focus Mode</span>
			<select class="select">
				<option value="manual-focus">Manual</option>
				<option value="automatic-auto-focus">Automatic</option>
				<option value="continuous-auto-focus">Continuous</option>
			</select>
		</label>
		<label class="label">
			<span>Auto Focus Range</span>
			<select class="select">
				<option value="manual-focus">Manual</option>
				<option value="automatic-auto-focus">Automatic</option>
				<option value="continuous-auto-focus">Continuous</option>
			</select>
		</label>
		<label class="label">
			<span>Lens Focus Position</span>
			<div class="input-group input-group-divider grid-cols-[1fr_auto]">
				<input class="input" type="number" value="20" min="1" max="200" />
				<div class="input-group-shim">cm</div>
			</div>
		</label>
		<label class="label">
			<span> Auto Focus Range</span>
			<select class="select"> </select>
		</label>
	</section>
	<footer class="card-footer"></footer>
</div>
