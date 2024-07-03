import { NetConn } from 'node-tcp';

export class NUTClient {
	private constructor(private conn: NetConn) {}

	static async connect(host: string, port: number): Promise<NUTClient> {
		const conn = await NetConn.connectToHost({ port: port, host: host }, false);
		return new NUTClient(conn);
	}

	async getVariable(client: string, variable: string): Promise<string> {
		await this.conn.writeString('GET VAR ' + client + ' ' + variable + '\n');
		const resp = await this.conn.readString();
		if (resp == null) {
			throw new Error('no response from server');
		}
		const prefix = 'VAR ' + client + ' ' + variable + ' "';
		if (resp.startsWith(prefix) == false) {
			throw new Error('error from server: ' + resp);
		}
		return resp.slice(prefix.length, -1);
	}
}
