import { createCheckers } from 'ts-interface-checker';
import { describe, expect, it } from 'vitest';
import typesTI from './types-ti';
import settingsTI from './settings-ti';
import { defaultSettings } from './settings';
describe('Settings', () => {
	const checker = createCheckers(typesTI, settingsTI);

	it('should have a default that implements interface', () => {
		expect(checker.check(defaultSettings)).toEqual(true);
	});
});
