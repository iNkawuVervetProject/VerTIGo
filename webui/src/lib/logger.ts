import { dev } from '$app/environment';

interface PartialLogger {
	info: typeof console.info;
	debug: typeof console.debug;
	error: typeof console.error;
}

export const logger: PartialLogger = dev
	? console
	: {
			info: () => {},
			debug: () => {},
			error: () => {}
		};
