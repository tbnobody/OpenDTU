export interface GridProfileValue {
    n: string;
    u: string;
    v: number;
}

export interface GridProfileSection {
    name: string;
    items: Array<GridProfileValue>;
}

export interface GridProfileStatus {
    wifi_inverter: boolean;
    name: string;
    version: string;
    sections: Array<GridProfileSection>;
}
