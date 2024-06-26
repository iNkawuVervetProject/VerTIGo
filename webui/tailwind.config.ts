import type { Config } from 'tailwindcss';
import { join } from 'path';

import { skeleton } from '@skeletonlabs/tw-plugin';
import forms from '@tailwindcss/forms';
import typography from '@tailwindcss/typography';
const config = {
	darkMode: 'selector',
	content: [
		'./src/**/*.{html,js,svelte,ts}',
		join(require.resolve('@skeletonlabs/skeleton'), '../**/*.{html,js,svelte,ts}')
	],

	theme: {
		extend: {}
	},

	plugins: [
		forms,
		typography,
		skeleton({ themes: { preset: [{ name: 'rocket', enhancements: true }] } })
	]
} satisfies Config;

export default config;
