export interface ValueObject {
    v: number; // value
    u: string; // unit
    d: number; // digits
};

export interface InverterStatistics {
    name: ValueObject,
    Power?: ValueObject;
    Voltage?: ValueObject;
    Current?: ValueObject;
    "Power DC"?: ValueObject;
    YieldDay?: ValueObject;
    YieldTotal?: ValueObject;
    Frequency?: ValueObject;
    Temperature?: ValueObject;
    PowerFactor?: ValueObject;
    ReactivePower?: ValueObject;
    Efficiency?: ValueObject;
    Irradiation?: ValueObject;
}

export interface Inverter {
    serial: number;
    name: string;
    data_age: number;
    reachable: boolean;
    producing: boolean;
    limit_relative: number;
    limit_absolute: number;
    events: number;
    [key: number]: InverterStatistics;
};

export interface Total {
    Power: ValueObject;
    YieldDay: ValueObject;
    YieldTotal: ValueObject;
};

export interface LiveData {
    inverters: Inverter[];
    total: Total;
}