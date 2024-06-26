<script lang="ts">
	import { parameters } from '$lib/parameters';
	import { Autocomplete } from '@skeletonlabs/skeleton';
	import type { AutocompleteOption } from '@skeletonlabs/skeleton';

	const participantOptions: AutocompleteOption<string>[] = [
		{ label: 'Asari', value: 'asari', keywords: 'female, biotic' },
		{ label: 'Krogan', value: 'krogan', keywords: 'violent, genophage' },
		{ label: 'Salarian', value: 'salarian', keywords: 'fast, scientist' },
		{ label: 'Quarian', value: 'quarian', keywords: 'fast, scientist' },
		{ label: 'Turian', value: 'turian', keywords: 'fast, scientist' },
		{ label: 'Vorcha', value: 'vorcha', keywords: 'fast, scientist' },
		{ label: 'Batarian', value: 'batarian', keywords: 'fast, scientist' }
	];

	let showSuggestions = false;
	let suggestions: any;

	function onParticipantSelection(event: CustomEvent<AutocompleteOption<string>>): void {
		$parameters.participant = event.detail.label;
		showSuggestions = false;
	}
</script>

<div class="relative">
	<input
		class="input max-w-sm"
		type="search"
		name="participant"
		bind:value={$parameters.participant}
		placeholder="Participant"
		on:focus={() => {
			showSuggestions = true;
		}}
		on:blur={(event) => {
			if (suggestions.contains(event.relatedTarget) && event.relatedTarget != suggestions) {
				return;
			}
			showSuggestions = false;
		}}
	/>

	<div
		class="card absolute top-12 z-10 max-h-48 w-full max-w-sm overflow-y-auto p-4"
		class:hidden={!showSuggestions}
		tabindex="-1"
		bind:this={suggestions}
	>
		<Autocomplete
			bind:input={$parameters.participant}
			options={participantOptions}
			on:selection={onParticipantSelection}
			emptyState={'New participant ' + $parameters.participant}
		/>
	</div>
</div>
