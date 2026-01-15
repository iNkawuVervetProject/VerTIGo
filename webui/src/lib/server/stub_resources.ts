import { type Group } from '$lib/types';

const groups: Group[] = [
	{
		name: 'Noah',
		idt: 'NH',
		individuals: [
			{
				name: 'Pomelo',
				idt: 'POM'
			},
			{
				name: 'Palmela',
				idt: 'PALM'
			}
		]
	},
	{
		name: 'Baie Dankie',
		idt: 'BD',
		individuals: [
			{ name: 'Braie', idt: 'BRA' },
			{ name: 'Piepra', idt: 'PIEP' }
		]
	}
];

export const resources = {
	groups: groups,
	fake: { name: 'coucou' }
};
