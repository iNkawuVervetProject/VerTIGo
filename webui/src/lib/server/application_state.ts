import { VertigoState } from '$lib/vertigo_state.ts~';
import { env } from '$env/dynamic/private';
import { readBatteryState } from './battery';

const BACKEND_HOST = env.BACKEND_HOST ?? 'localhost:5000';

export class ApplicationState extends VertigoState {
	private _source: EventSource;

	private _timeout: any = undefined;

	public constructor(private _eventURL: string) {
		super();
		this._connect();

		setInterval(async () => {
			const state = await readBatteryState();
			if (state instanceof Error) {
				console.error(state.toString());
				this._battery.set({});
			} else {
				this._battery.set(state);
			}
		}, 15000);
	}

	public _connect() {
		if (this._timeout !== undefined) {
			clearInterval(this._timeout);
		}

		console.log(`connection to ${this._eventURL}`);

		this._source = new EventSource(this._eventURL);

		this._source.onerror = (evt) => {
			console.error(`got event-stream error: ${evt}`);
			this._scheduleReconnect();
		};

		this._catalog.set({});
		this.setEventSource(this._source);
	}

	public _scheduleReconnect() {
		if (this._timeout !== undefined) {
			return;
		}

		this._timeout = setTimeout(() => {
			this._timeout = undefined;
			this._connect();
		}, 2000);
	}
}

export const appication_state = new ApplicationState(`http://${BACKEND_HOST}/events`);
