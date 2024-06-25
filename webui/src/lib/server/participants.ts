import { Participant } from '$lib/types';
import { LOCAL_STORAGE_DIR } from '$env/static/private';

import * as fs from 'fs';

type ParticipantByName = { [key: string]: Participant };

class ParticipantService {
	public constructor(public participants: ParticipantByName) {}

	static localPath: string = LOCAL_STORAGE_DIR + '/participants.json';

	static async OpenLocal(): Promise<ParticipantService> {
		const content = await fs.readFile(ParticipantService.localPath);
		return new ParticipantService(JSON.parse(content));
	}

	private async saveLocal(): Promise<void> {
		await fs.writeFile(ParticipantService.localPath, JSON.stringify(this.participants));
	}

	async participant(name: string): Promise<Participant> {
		if (!(name in this.participants)) {
			this.participants[name] = new Participant(name, 0);
			await this.saveLocal();
		}
		return this.participants[name];
	}

	async incrementParticipantSession(name: string): Promise<void> {
		if (!(name in this.participants)) {
			this.participants[name] = new Participant(name, 1);
		} else {
			this.participants[name].sessions += 1;
		}
		await this.saveLocal();
	}
}

export const participantService: ParticipantService = await ParticipantService.OpenLocal();
