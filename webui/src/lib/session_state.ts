import { writable, type Writable } from 'svelte/store';
import { type Catalog } from './types';
import { onMount } from 'svelte';

let subscribed = false;

export const window: Writable<boolean> = writable<boolean>(false);
export const catalog: Writable<Catalog> = writable<Catalog>({});
export const experiment: Writable<string> = writable<string>('');
export const messages: Writable<any[]> = writable<any[]>([]);

export function synchronizeState(): void {
	onMount(() => {
		console.log('synchronizing....');
		if (subscribed) {
			console.log('already synchronized');
			return;
		}
		subscribed = true;
		const eventSource = new EventSource('/psysw/api/events');
		eventSource.onmessage = (event) => {
			const data = JSON.parse(event.data);
			messages.update((arr: any[]) => arr.concat(data));
		};
		eventSource.addEventListener('windowUpdate', (event) => {
			const data = JSON.parse(event.data);
			window.set(data);
		});
		eventSource.addEventListener('experimentUpdate', (event) => {
			experiment.set(JSON.parse(event.data));
		});
		eventSource.addEventListener('catalogUpdate', (event) => {
			catalog.set(JSON.parse(event.data));
		});
	});
}
