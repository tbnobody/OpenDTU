export interface ValueObject {
    v: number, // value
    u: string, // unit
    d: number, // digits
};

export interface InverterStatistics {
    Power?: ValueObject,
    Voltage?: ValueObject,
    Current?: ValueObject,
    "Power DC"?: ValueObject,
    YieldDay?: ValueObject,
    YieldTotal?: ValueObject,
    Frequency?: ValueObject,
    Temperature?: ValueObject,
    PowerFactor?: ValueObject,
    ReactivePower?: ValueObject,
    Efficiency?: ValueObject,
    Irradiation?: ValueObject,
}

export interface Inverter {
    serial: number,
    name: string,
    data_age: number,
    reachable: boolean,
    producing: boolean,
    limit_relative: number,
    limit_absolute: number,
    events: number,
    [key: number]: InverterStatistics,
};

export interface Total {
    Power: ValueObject,
    YieldDay: ValueObject,
    YieldTotal: ValueObject,
};

export interface LiveData {
    inverters: Inverter[],
    total: Total,
}

// Ve.Direct 
export interface Vedirect {
    SER: string,
    PID: string,
    FW: string,
    age_critical: boolean,
    data_age: 0,
    LOAD: ValueObject,
    CS: ValueObject,
    MPPT: ValueObject,
    OR: ValueObject,
    ERR: ValueObject,
    HSDS: ValueObject,
    V: ValueObject,
    I: ValueObject,
    VPV: ValueObject,
    PPV: ValueObject,
    H19: ValueObject,
    H20: ValueObject,
    H21: ValueObject,
    H22: ValueObject,
    H23: ValueObject,
}