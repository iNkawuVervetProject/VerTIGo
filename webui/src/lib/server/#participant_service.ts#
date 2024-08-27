import { LOCAL_STORAGE_DIR } from '$env/static/private';
import { Participant } from '$lib/types';
import { readFile, writeFile } from 'fs';
import { ParticipantByName, participantService } from './participants';

export class ParticipantService {
	public constructor(private participants: ParticipantByName) {}

	static localPath: string = LOCAL_STORAGE_DIR + '/participants.json';

	static async OpenLocal(): ParticipantService {
		const content = await readFile(ParticipantService.localPath);
		return new ParticipantService(JSON.parse(content));
	}

	private async saveLocal(): void {
		await writeFile(participantService.localPath, JSON.stringify(this.participants));
	}

	participant(name: string): Participant {
		if (!(name in this.participants)) {
			this.participants[name] = new Participant(name, 0);
			this.saveLocal();
		}
		return this.participants[name];
	}

	incrementParticipantSession(name: string) {
		this.participant(name).sessions += 1;
		this.saveLocal();
	}
}
