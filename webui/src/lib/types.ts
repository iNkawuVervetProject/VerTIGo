export interface Catalog {
	[key: string]: Experiment;
}

export interface Parameters {
	participant: string;
	session: number;
	[key: string]: string | number;
}

export interface Experiment {
	key: string;
	resources: { [key: string]: boolean };
	parameters: string[];
}
