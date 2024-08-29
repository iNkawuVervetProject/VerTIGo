import { env } from '$env/dynamic/private';

export const CAMERA_HOST = env.CAMERA_HOST ?? 'localhost:5042';
export const CAMERA_URL = `http://${CAMERA_HOST}/camera`;

export const BACKEND_HOST = env.BACKEND_HOST ?? 'localhost:5000';

export const MEDIAMTX_HOST = env.MEDIAMTX_HOST ?? 'localhost:8889';

export const NUT_HOSTNAME = env.NUT_HOSTNAME || 'localhost';

export const DEBUG = (env.DEBUG ?? '0') != '0';
