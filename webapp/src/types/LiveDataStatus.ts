export interface ValueObject {
    v: number; // value
    u: string; // unit
    d: number; // digits
    max: number;
}

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
    order: number;
    data_age: number;
    poll_enabled: boolean;
    reachable: boolean;
    producing: boolean;
    limit_relative: number;
    limit_absolute: number;
    events: number;
    AC: InverterStatistics[];
    DC: InverterStatistics[];
    INV: InverterStatistics[];
}

export interface Total {
    Power: ValueObject;
    YieldDay: ValueObject;
    YieldTotal: ValueObject;
}

export interface Hints {
    time_sync: boolean;
    default_password: boolean;
    radio_problem: boolean;
}

export interface LiveData {
    inverters: Inverter[];
    total: Total;
    hints: Hints;
}