import { purgeCss } from 'vite-plugin-tailwind-purgecss';
import { sveltekit } from '@sveltejs/kit/vite';
import { defineConfig } from 'vitest/config';

import { exec } from 'child_process';
import utils from 'util';

const execute = utils.promisify(exec);

function runInterfaceBuilder({ files }) {
	return {
		name: 'run-ts-interface-builder',
		async buildStart(options) {
			if (files.length == 0) {
				return;
			}
			await execute(`./node_modules/.bin/ts-interface-builder ${files}`);
		}
	};
}

export default defineConfig({
	plugins: [
		sveltekit(),
		purgeCss(),
		runInterfaceBuilder({ files: 'src/lib/settings.ts src/lib/types.ts' })
	],
	test: {
		include: ['src/**/*.{test,spec}.{js,ts}']
	}
});
