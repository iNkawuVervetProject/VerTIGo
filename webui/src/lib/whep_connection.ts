import { dev } from '$app/environment';
import { readonly, writable, type Readable, type Unsubscriber, type Writable } from 'svelte/store';

interface PartialLogger {
	info: typeof console.info;
	debug: typeof console.debug;
	error: typeof console.error;
}

const logger: PartialLogger = dev
	? console
	: {
			info: () => {},
			debug: () => {},
			error: () => {}
		};

function supportsNonAdvertisedCodec(codec: string, fmtp?: string): Promise<boolean> {
	return new Promise((resolve) => {
		const pc = new RTCPeerConnection({ iceServers: [] });
		pc.addTransceiver('audio', { direction: 'recvonly' });
		pc.createOffer()
			.then((offer) => {
				if (offer.sdp?.includes(' ' + codec)) {
					// codec is advertised, there's no need to add it manually
					resolve(false);
					return;
				}
				const sections = offer.sdp?.split('m=audio');
				if (sections === undefined || sections.length < 2) {
					resolve(false);
					return;
				}
				const lines = sections[1].split('\r\n');
				lines[0] += ' 118';
				lines.splice(lines.length - 1, 0, 'a=rtpmap:118 ' + codec);
				if (fmtp !== undefined) {
					lines.splice(lines.length - 1, 0, 'a=fmtp:118 ' + fmtp);
				}
				sections[1] = lines.join('\r\n');
				offer.sdp = sections.join('m=audio');
				return pc.setLocalDescription(offer);
			})
			.then(() => {
				return pc.setRemoteDescription(
					new RTCSessionDescription({
						type: 'answer',
						sdp:
							'v=0\r\n' +
							'o=- 6539324223450680508 0 IN IP4 0.0.0.0\r\n' +
							's=-\r\n' +
							't=0 0\r\n' +
							'a=fingerprint:sha-256 0D:9F:78:15:42:B5:4B:E6:E2:94:3E:5B:37:78:E1:4B:54:59:A3:36:3A:E5:05:EB:27:EE:8F:D2:2D:41:29:25\r\n' +
							'm=audio 9 UDP/TLS/RTP/SAVPF 118\r\n' +
							'c=IN IP4 0.0.0.0\r\n' +
							'a=ice-pwd:7c3bf4770007e7432ee4ea4d697db675\r\n' +
							'a=ice-ufrag:29e036dc\r\n' +
							'a=sendonly\r\n' +
							'a=rtcp-mux\r\n' +
							'a=rtpmap:118 ' +
							codec +
							'\r\n' +
							(fmtp !== undefined ? 'a=fmtp:118 ' + fmtp + '\r\n' : '')
					})
				);
			})
			.then(() => {
				resolve(true);
			})
			.catch(() => {
				resolve(false);
			})
			.finally(() => {
				pc.close();
			});
	});
}

async function getNonAdvertisedCodecs(): Promise<string[]> {
	return await Promise.all(
		[
			['pcma/8000/2'],
			['multiopus/48000/6', 'channel_mapping=0,4,1,2,3,5;num_streams=4;coupled_streams=2'],
			['L16/48000/2']
		]
			.filter(async ([codec, fmtp]) => await supportsNonAdvertisedCodec(codec, fmtp))
			.map((c) => c[0])
	);
}

interface ICEServer {
	urls: string[];
	username?: string;
	credential?: string;
	credentialType?: string;
}

function unquoteCredential(v: string) {
	return JSON.parse(`"${v}"`);
}

function linkToIceServers(links: string): ICEServer[] {
	if (links === null) {
		return [];
	}

	return links.split(', ').map((link): ICEServer => {
		const m = link.match(
			/^<(.+?)>; rel="ice-server"(; username="(.*?)"; credential="(.*?)"; credential-type="password")?/i
		);
		if (m === null) {
			throw Error('no match found');
		}
		const ret: ICEServer = {
			urls: [m[1]]
		};

		if (m[3] !== undefined) {
			ret.username = unquoteCredential(m[3]);
			ret.credential = unquoteCredential(m[4]);
			ret.credentialType = 'password';
		}

		return ret;
	});
}

interface SDPOffer {
	iceUfrag: string;
	icePwd: string;
	medias: string[];
}

function parseOffer(sdp: string): SDPOffer {
	const ret: SDPOffer = {
		iceUfrag: '',
		icePwd: '',
		medias: []
	};

	sdp.split('\r\n').forEach((line) => {
		if (line.startsWith('m=')) {
			ret.medias.push(line.slice('m='.length));
		} else if (ret.iceUfrag === '' && line.startsWith('a=ice-ufrag:')) {
			ret.iceUfrag = line.slice('a=ice-ufrag:'.length);
		} else if (ret.icePwd === '' && line.startsWith('a=ice-pwd:')) {
			ret.icePwd = line.slice('a=ice-pwd:'.length);
		}
	});

	return ret;
}

function enableStereoPcmau(section: string): string {
	let lines = section.split('\r\n');

	lines[0] += ' 118';
	lines.splice(lines.length - 1, 0, 'a=rtpmap:118 PCMU/8000/2');
	lines.splice(lines.length - 1, 0, 'a=rtcp-fb:118 transport-cc');

	lines[0] += ' 119';
	lines.splice(lines.length - 1, 0, 'a=rtpmap:119 PCMA/8000/2');
	lines.splice(lines.length - 1, 0, 'a=rtcp-fb:119 transport-cc');

	return lines.join('\r\n');
}

function enableMultichannelOpus(section: string): string {
	let lines = section.split('\r\n');

	lines[0] += ' 112';
	lines.splice(lines.length - 1, 0, 'a=rtpmap:112 multiopus/48000/3');
	lines.splice(
		lines.length - 1,
		0,
		'a=fmtp:112 channel_mapping=0,2,1;num_streams=2;coupled_streams=1'
	);
	lines.splice(lines.length - 1, 0, 'a=rtcp-fb:112 transport-cc');

	lines[0] += ' 113';
	lines.splice(lines.length - 1, 0, 'a=rtpmap:113 multiopus/48000/4');
	lines.splice(
		lines.length - 1,
		0,
		'a=fmtp:113 channel_mapping=0,1,2,3;num_streams=2;coupled_streams=2'
	);
	lines.splice(lines.length - 1, 0, 'a=rtcp-fb:113 transport-cc');

	lines[0] += ' 114';
	lines.splice(lines.length - 1, 0, 'a=rtpmap:114 multiopus/48000/5');
	lines.splice(
		lines.length - 1,
		0,
		'a=fmtp:114 channel_mapping=0,4,1,2,3;num_streams=3;coupled_streams=2'
	);
	lines.splice(lines.length - 1, 0, 'a=rtcp-fb:114 transport-cc');

	lines[0] += ' 115';
	lines.splice(lines.length - 1, 0, 'a=rtpmap:115 multiopus/48000/6');
	lines.splice(
		lines.length - 1,
		0,
		'a=fmtp:115 channel_mapping=0,4,1,2,3,5;num_streams=4;coupled_streams=2'
	);
	lines.splice(lines.length - 1, 0, 'a=rtcp-fb:115 transport-cc');

	lines[0] += ' 116';
	lines.splice(lines.length - 1, 0, 'a=rtpmap:116 multiopus/48000/7');
	lines.splice(
		lines.length - 1,
		0,
		'a=fmtp:116 channel_mapping=0,4,1,2,3,5,6;num_streams=4;coupled_streams=4'
	);
	lines.splice(lines.length - 1, 0, 'a=rtcp-fb:116 transport-cc');

	lines[0] += ' 117';
	lines.splice(lines.length - 1, 0, 'a=rtpmap:117 multiopus/48000/8');
	lines.splice(
		lines.length - 1,
		0,
		'a=fmtp:117 channel_mapping=0,6,1,4,5,2,3,7;num_streams=5;coupled_streams=4'
	);
	lines.splice(lines.length - 1, 0, 'a=rtcp-fb:117 transport-cc');

	return lines.join('\r\n');
}

function enableL16(section: string): string {
	let lines = section.split('\r\n');

	lines[0] += ' 120';
	lines.splice(lines.length - 1, 0, 'a=rtpmap:120 L16/8000/2');
	lines.splice(lines.length - 1, 0, 'a=rtcp-fb:120 transport-cc');

	lines[0] += ' 121';
	lines.splice(lines.length - 1, 0, 'a=rtpmap:121 L16/16000/2');
	lines.splice(lines.length - 1, 0, 'a=rtcp-fb:121 transport-cc');

	lines[0] += ' 122';
	lines.splice(lines.length - 1, 0, 'a=rtpmap:122 L16/48000/2');
	lines.splice(lines.length - 1, 0, 'a=rtcp-fb:122 transport-cc');

	return lines.join('\r\n');
}

function enableStereoOpus(section: string): string {
	let opusPayloadFormat = '';
	let lines = section.split('\r\n');

	for (let i = 0; i < lines.length; i++) {
		if (lines[i].startsWith('a=rtpmap:') && lines[i].toLowerCase().includes('opus/')) {
			opusPayloadFormat = lines[i].slice('a=rtpmap:'.length).split(' ')[0];
			break;
		}
	}

	if (opusPayloadFormat === '') {
		return section;
	}

	for (let i = 0; i < lines.length; i++) {
		if (lines[i].startsWith('a=fmtp:' + opusPayloadFormat + ' ')) {
			if (!lines[i].includes('stereo')) {
				lines[i] += ';stereo=1';
			}
			if (!lines[i].includes('sprop-stereo')) {
				lines[i] += ';sprop-stereo=1';
			}
		}
	}

	return lines.join('\r\n');
}

function generateSdpFragment(offer: SDPOffer, candidates: RTCIceCandidate[]) {
	interface CandidateByMedia {
		[key: number]: RTCIceCandidate[];
	}

	const candidatesByMedia: CandidateByMedia = {};
	for (const candidate of candidates) {
		const mid = candidate.sdpMLineIndex;
		if (mid === null) {
			continue;
		}

		if (candidatesByMedia[mid] === undefined) {
			candidatesByMedia[mid] = [];
		}
		candidatesByMedia[mid].push(candidate);
	}

	let frag = 'a=ice-ufrag:' + offer.iceUfrag + '\r\n' + 'a=ice-pwd:' + offer.icePwd + '\r\n';

	let mid = 0;

	for (const media of offer.medias) {
		if (candidatesByMedia[mid] !== undefined) {
			frag += 'm=' + media + '\r\n' + 'a=mid:' + mid + '\r\n';

			for (const candidate of candidatesByMedia[mid]) {
				frag += 'a=' + candidate.candidate + '\r\n';
			}
		}
		mid++;
	}

	return frag;
}

class Connection {
	public _source: Writable<MediaProvider | undefined> = writable<MediaProvider | undefined>(
		undefined
	);
	public _error: Writable<Error | undefined> = writable<Error | undefined>(undefined);

	private static _nonAdvertisedCodecs?: string[] = undefined;

	private _connection: RTCPeerConnection | undefined = undefined;
	private _data?: SDPOffer = undefined;

	private _queuedCandidates: RTCIceCandidate[] = [];
	private _sessionURL: string = '';

	private _unsubscribe: Unsubscriber;

	private constructor(private _url: string) {
		logger.info(`[WHEPConnection:${this._url}]: create`);
		this._unsubscribe = this._error.subscribe((err?: Error) => {
			if (err !== undefined) {
				this.close();
			}
		});
	}

	public close(): void {
		logger.info(`[WHEPConnection:${this._url}]: closing`);
		this._unsubscribe();
		this._source.set(undefined);
		if (this._connection !== undefined) {
			this._connection.close();
			this._connection = undefined;
		}

		if (this._sessionURL) {
			fetch(this._sessionURL, { method: 'DELETE' });
			this._sessionURL = '';
		}

		this._queuedCandidates = [];
	}

	public static async connect(url: string): Promise<Connection> {
		if (Connection._nonAdvertisedCodecs === undefined) {
			Connection._nonAdvertisedCodecs = await getNonAdvertisedCodecs();
			logger.info(`[WHEPConnection]: nonAdvertisedCodecs`, Connection._nonAdvertisedCodecs);
		}

		const res = new Connection(url);
		try {
			await res._connect();
		} catch (err: any) {
			logger.info(`[WHEPConnection]: connection error: ${err}`);
			res.close();
			throw err;
		}
		return res;
	}

	private async _connect() {
		logger.info(`[WHEPConnection:${this._url}]: connect`);
		const resp = await fetch(this._url, { method: 'OPTIONS' });

		const iceServer = linkToIceServers(resp.headers.get('Link') || '');

		logger.debug(`[WHEPConnection:${this._url}]: iceServer`, iceServer);

		this._connection = new RTCPeerConnection({
			iceServers: iceServer,
			// https://webrtc.org/getting-started/unified-plan-transition-guide
			sdpSemantics: 'unified-plan'
		});

		const direction = 'sendrecv';
		this._connection.addTransceiver('video', { direction });
		this._connection.addTransceiver('audio', { direction });

		this._connection.onicecandidate = (evt: RTCPeerConnectionIceEvent) =>
			this._onLocalCandidate(evt);
		this._connection.oniceconnectionstatechange = () => this._onConnectionState();
		this._connection.ontrack = (evt: RTCTrackEvent) => {
			this._source.set(evt.streams[0]);
		};

		try {
			const offer = await this._connection.createOffer();

			offer.sdp = Connection._editOffer(offer.sdp || '');
			this._data = parseOffer(offer.sdp);
			await this._connection.setLocalDescription(offer);
			await this._sendOffer(offer);
		} catch (err: any) {
			this._error.set(err);
			throw err;
		}
	}

	private static _editOffer(sdp: string): string {
		return sdp
			.split('m=')
			.map((section: string) => {
				if (section.startsWith('audio') === false) {
					return section;
				}
				section = enableStereoOpus(section);

				if (Connection._nonAdvertisedCodecs?.includes('pcma/8000/2')) {
					section = enableStereoPcmau(section);
				}

				if (Connection._nonAdvertisedCodecs?.includes('multiopus/48000/6')) {
					section = enableMultichannelOpus(section);
				}

				if (Connection._nonAdvertisedCodecs?.includes('L16/48000/2')) {
					section = enableL16(section);
				}
			})
			.join('m=');
	}

	private async _sendOffer(offer: RTCSessionDescriptionInit) {
		logger.debug(`[WHEPConnection:${this._url}]: sendOffer`, offer);
		try {
			const resp = await fetch(this._url, {
				method: 'POST',
				headers: {
					'Content-Type': 'application/sdp'
				},
				body: offer.sdp
			});

			switch (resp.status) {
				case 201:
					break;
				case 404:
					throw new Error('stream not found');
				case 400:
					const body = await resp.json();
					throw new Error(body.error);
				default:
					throw new Error(`bad status code ${resp.status}`);
			}

			this._sessionURL = new URL(resp.headers.get('location') || '').toString();

			await this._onRemoteAnswer(await resp.text());
		} catch (err: any) {
			this._error.set(err);
			throw err;
		}
	}

	private async _onRemoteAnswer(sdp: string) {
		logger.debug(`[WHEPConnection:${this._url}]: onRemoteAnswer`, sdp);
		await this._connection?.setRemoteDescription(
			new RTCSessionDescription({
				type: 'answer',
				sdp
			})
		);

		if (this._queuedCandidates.length != 0) {
			this._sendLocalCandidates(this._queuedCandidates);
			this._queuedCandidates = [];
		}
	}

	private async _onLocalCandidate(evt: RTCPeerConnectionIceEvent) {
		logger.debug(`[WHEPConnection:${this._url}]: onLocalCandidate`, evt);
		if (evt.candidate === null) {
			return;
		}

		if (this._sessionURL === '') {
			this._queuedCandidates.push(evt.candidate);
		} else {
			try {
				await this._sendLocalCandidates([evt.candidate]);
			} catch (err: any) {
				this._error.set(err);
			}
		}
	}

	private async _sendLocalCandidates(candidates: RTCIceCandidate[]) {
		logger.debug(`[WHEPConnection:${this._url}]: sendLocalCandidate`, candidates);
		const resp = await fetch(this._sessionURL, {
			method: 'PATCH',
			headers: {
				'Content-Type': 'application/trickle-ice-sdpfrag',
				'If-Match': '*'
			},
			body: generateSdpFragment(
				this._data || { iceUfrag: '', icePwd: '', medias: [] },
				candidates
			)
		});
		switch (resp.status) {
			case 204:
				return;
			case 404:
				throw new Error('stream not found');
			default:
				throw new Error(`bad status code ${resp.status}`);
		}
	}

	private _onConnectionState() {
		logger.debug(
			`[WHEPConnection:${this._url}]: onConnectionState`,
			this._connection?.iceConnectionState
		);
		if (this._connection?.iceConnectionState === 'disconnected') {
			this._error.set(new Error('peer connection closed'));
		}
	}
}

export interface WHEPConnection {
	close: () => void;
	error: Readable<Error | undefined>;
	media: Readable<MediaProvider | undefined>;
}

export async function read_whep(url: string): Promise<WHEPConnection> {
	const conn = await Connection.connect(url);
	return {
		close: conn.close,
		error: readonly(conn._error),
		media: readonly(conn._source)
	};
}
