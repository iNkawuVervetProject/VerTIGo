<script lang="ts">
	import { parameters } from '$lib/parameters';
	import { participants } from '$lib/application_state';
	import { Autocomplete } from '@skeletonlabs/skeleton';
	import type { AutocompleteOption } from '@skeletonlabs/skeleton';

	$: participantOptions = Object.entries($participants).map(
		([key, p]) =>
			({
				label: `<p>${key}</p><p class="text-right text-sm italic w-full"">(next:${p.nextSession})</p>`,
				value: p.name,
				meta: { nextSession: p.nextSession }
			}) as AutocompleteOption<string>
	);
	let showSuggestions = false;
	let suggestions: any;

	function onParticipantSelection(event: CustomEvent<AutocompleteOption<string>>): void {
		$parameters.participant = event.detail.value;
		showSuggestions = false;
	}
</script>

<div class="relative">
	<input
		class="input md:max-w-sm"
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
		class="card absolute top-12 z-10 max-h-48 w-full overflow-y-auto p-4 md:max-w-sm"
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
