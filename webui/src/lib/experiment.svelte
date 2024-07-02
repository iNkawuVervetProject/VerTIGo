<script lang="ts">
	import type { Experiment, Participant } from '$lib/types';
	import { parameters } from './parameters';
	import { experiment as currentExperiment, participants } from './session_state';
	import { getModalStore, type ModalSettings } from '@skeletonlabs/skeleton';

	export let experiment: Experiment;

	const modalStore = getModalStore();

	async function confirmStop(key: string): Promise<boolean> {
		return new Promise<boolean>((resolve) => {
			const modal: ModalSettings = {
				type: 'confirm',
				title: 'Please Confirm',
				body: `Do you wish to stop ${key}?`,
				response: (r: boolean) => {
					resolve(r);
				}
			};
			modalStore.trigger(modal);
		});
	}

	async function confirmStart(participant: Participant, session: number): Promise<boolean> {
		return new Promise<boolean>((resolve) => {
			const modal: ModalSettings = {
				type: 'confirm',
				title: 'Please Confirm Override Session',
				body: `Pariticipant ${participant.name} may already have recorded session ${session}. Do you wish to continue?`,
				response: (r: boolean) => {
					resolve(r);
				}
			};
			modalStore.trigger(modal);
		});
	}

	async function start() {
		const params = Object.assign(
			{},
			...experiment.parameters.map((p: string) => ({
				[p]: $parameters[p]
			}))
		);
		if ($parameters.session != undefined && $parameters.participant != undefined) {
			const session = $parameters.session;
			const p = $participants[$parameters.participant];
			if (p != undefined && session < p.nextSession) {
				const confirm = await confirmStart(p, session);
				if (confirm == false) {
					return;
				}
			}
		}

		await fetch('/psysw/api/experiment', {
			method: 'POST',
			body: JSON.stringify({ key: experiment.key, parameters: params })
		});
	}

	async function stop() {
		const confirm = await confirmStop($currentExperiment);
		if (confirm == false) {
			return;
		}
		await fetch('/psysw/api/experiment', { method: 'DELETE' });
	}

	$: missingResources = Object.entries(experiment.resources)
		.filter(([_, valid]: [string, boolean]) => !valid)
		.map(([path, _]) => path);

	$: missingParameters = experiment.parameters?.filter(
		(p) => !(p in $parameters) || (p == 'participant' && $parameters.participant?.length === 0)
	);
	$: canRun =
		$currentExperiment === '' &&
		missingParameters.length === 0 &&
		missingResources.length === 0;
	$: isRunning = $currentExperiment === experiment.key;
</script>

<section
	class="card min-h-24 w-full space-y-4 p-8"
	class:variant-ghost-surface={!isRunning}
	class:variant-ghost-secondary={isRunning}
>
	<div class="flex">
		<h1 class="flex-auto text-2xl">{experiment.key}</h1>
		<button
			class="btn-icon justify-self-end"
			class:variant-filled-error={isRunning}
			class:variant-filled-success={!isRunning}
			disabled={!canRun && !isRunning}
			on:click={() => {
				if (isRunning) {
					stop();
				} else {
					start();
				}
			}}
		>
			<i class="fa-solid" class:fa-play={!isRunning} class:fa-stop={isRunning} />
		</button>
	</div>
	<dl class="list-dl">
		{#each missingResources as r}
			<div>
				<span class="variant-soft-error badge-icon p-4"
					><i class="fa-solid fa-file-circle-question" /></span
				>
				<span class="flex-auto">
					<dt class="font-bold">Missing resource</dt>
					<dd class="text-sm opacity-50">{r}</dd>
				</span>
			</div>
		{/each}
		{#each missingParameters as p}
			<div>
				<span class="variant-soft-warning badge-icon p-4"
					><i class="fa-solid fa-triangle-exclamation" /></span
				>
				<span class="flex-auto">
					<dt class="font-bold">Missing parameter</dt>
					<dd class="text-sm opacity-50">{p}</dd>
				</span>
			</div>
		{/each}
	</dl>
</section>
