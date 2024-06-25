import { derived, writable, type Readable, type Writable } from 'svelte/store';
import type { Parameters, Catalog } from './types';
import { catalog } from './session_state';

export const parameters: Writable<Partial<Parameters>> = writable<Partial<Parameters>>({});

export const maybeRequiredParameters: Readable<string[]> = derived<Readable<Catalog>, string[]>(
	catalog,
	($catalog) => {
		const result = new Set(['participant', 'session']);
		Object.entries($catalog).forEach(([_, experiment]) => {
			experiment.parameters.forEach((p) => result.add(p));
		});
		return Array.from(result);
	}
);
