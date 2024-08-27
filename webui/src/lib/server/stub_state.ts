import { Participant, type Catalog, type Parameters, type ParticipantByName } from '$lib/types';
import { server } from '$lib/application_state';
import { get, readable } from 'svelte/store';

export const catalog: Catalog = {
	'valid.psyexp': {
		key: 'valid.psyexp',
		resources: { 'somepic.png': true },
		parameters: ['participant', 'session']
	},
	'unvalid.psyexp': {
		key: 'unvalid.psyexp',
		resources: { 'missing.png': false, 'missing_again.png': false },
		parameters: ['participant', 'session']
	},
	'more.psyexp': {
		key: 'more.psyexp',
		resources: { 'somepic.png': true },
		parameters: ['participant', 'session', 'age']
	}
};

export const participants: ParticipantByName = Object.assign(
	{},
	...[
		new Participant('asari', 123453),
		new Participant('turian', 1),
		new Participant('salarian', 42)
	].map((p: Participant) => ({ [p.name]: p }))
);

let _timeout: ReturnType<typeof setTimeout> | undefined = undefined;

export function openWindow(): void {
	server.window?.set(true);
}

export function initFakeData(): void {
	server.experiment?.set('');
	server.catalog?.set(catalog);
	server.participants?.set(participants);
}

export function runExperiment(key: string, parameters: Parameters): void {
	let exp = catalog[key];
	if (exp === undefined) {
		throw new Error(`unknown experiment '${key}'`);
	}
	let currExperiment = get(server.experiment || readable(''));
	if (currExperiment !== '') {
		throw new Error(`experiment '${currExperiment}' is already running`);
	}

	openWindow();
	server.experiment?.set(key);
	const participant = parameters.participant;
	if (!(participant in participants)) {
		participants[participant] = new Participant(participant, 0);
	}
	participants[participant].nextSession = Math.max(
		participants[participant].nextSession,
		parameters.session + 1
	);
	server.participants?.set(participants);
	//experiment finishes after 10s
	_timeout = setTimeout(() => {
		_stopExperiment();
	}, 10000);
}

export function stopExperiment(): void {
	if (_timeout === undefined) {
		throw new Error('no experiment started');
	}
	_stopExperiment();
}

function _stopExperiment(): void {
	if (_timeout !== undefined) {
		clearTimeout(_timeout);
	}
	_timeout = undefined;
	server.experiment?.set('');
}

export function closeWindow(): void {
	if (get(server.window || readable(false)) == false) {
		throw new Error('window is not opened');
	}
	server.window?.set(false);
}
