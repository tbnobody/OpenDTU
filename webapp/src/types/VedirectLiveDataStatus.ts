import type { ValueObject } from '@/types/LiveDataStatus';

export interface DynamicPowerLimiter {
    PLSTATE: number;
    PLLIMIT: number;
}

export interface VedirectDevice {
    SER: string;
    PID: string;
    FW: string;
    age_critical: boolean;
    data_age: 0;
    LOAD: ValueObject;
    CS: ValueObject;
    MPPT: ValueObject;
    OR: ValueObject;
    ERR: ValueObject;
    HSDS: ValueObject;
}

export interface VedirectOutput {
    P: ValueObject;
    V: ValueObject;
    I: ValueObject;
    E: ValueObject;
}

export interface VedirectInput {
    PPV: ValueObject;
    VPV: ValueObject;
    IPV: ValueObject;
    H19: ValueObject;
    H20: ValueObject;
    H21: ValueObject;
    H22: ValueObject;
    H23: ValueObject;
}
