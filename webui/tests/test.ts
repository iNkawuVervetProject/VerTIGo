import { expect, test } from '@playwright/test';

test('home page has expected VERTIGO title', async ({ page }) => {
	await page.goto('/');
});
