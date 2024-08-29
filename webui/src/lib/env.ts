import { dev } from '$app/environment';
import { PUBLIC_NO_LOCAL_DEV_ENDPOINT } from '$env/static/public';

export const FAKE_BACKEND = PUBLIC_NO_LOCAL_DEV_ENDPOINT === '0' && dev;
