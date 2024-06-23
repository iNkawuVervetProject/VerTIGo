import { PUBLIC_NO_LOCAL_DEV_ENDPOINT } from '$env/static/public';
import { dev } from '$app/environment';

export function match(param: string) {
	return PUBLIC_NO_LOCAL_DEV_ENDPOINT === '0' && dev;
}
