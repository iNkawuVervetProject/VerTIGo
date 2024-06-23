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
		if (subscribed) {
			return;
		}
		subscribed = true;
		const eventSource = new EventSource('https://sse.dev/test');
		eventSource.onmessage = (event) => {
			const data = JSON.parse(event.data);

			messages.update((arr: any[]) => {
				return arr.concat(data);
			});
		};
	});
}
