export interface Catalog {
	[key: string]: Experiment;
}

export interface Parameters {
	[key: string]: any;
}

export interface Experiment {
	key: string;
	resources: { [key: string]: boolean };
	parameters: Parameters;
}
