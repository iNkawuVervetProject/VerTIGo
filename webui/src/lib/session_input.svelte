<script lang="ts">
	import { parameters } from './parameters';
	import { participants } from './application_state';
	import type { ParticipantByName } from './types';

	let inputValue: number = 1;
	let manual: boolean = false;

	function fromState(p: string, participants: ParticipantByName): number {
		if (p in participants) {
			return participants[p].nextSession;
		}
		return 1;
	}

	$: {
		if (manual == true) {
			$parameters.session = inputValue;
		} else {
			$parameters.session = fromState($parameters.participant || '', $participants);
			inputValue = $parameters.session;
		}
	}
</script>

<div class="flex items-center gap-2 md:max-w-sm">
	<input class="checkbox" type="checkbox" name="manual" bind:checked={manual} />
	<div class="input-group input-group-divider w-full grow grid-cols-[auto_1fr_auto]">
		<div class="input-group-shim">session</div>
		<input
			class="input"
			type="number"
			name="session"
			bind:value={inputValue}
			disabled={!manual}
		/>
	</div>
</div>
