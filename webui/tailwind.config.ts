import type { Config } from 'tailwindcss';
import { join } from 'path';

import { skeleton } from '@skeletonlabs/tw-plugin';

const config = {
	darkMode: 'class',
	content: [
		'./src/**/*.{html,js,svelte,ts}',
		join(require.resolve('@skeletonlabs/skeleton'), '../**/*.{html,js,svelte,ts}')
	],

	theme: {
		extend: {}
	},

	plugins: ['@tailwindcss/typography', skeleton({ themes: { preset: ['skeleton'] } })]
} satisfies Config;

export default config;
