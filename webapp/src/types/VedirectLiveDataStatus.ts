import type { ValueObject } from '@/types/LiveDataStatus';

export interface DynamicPowerLimiter {
    PLSTATE: number;
    PLLIMIT: number;
}

export interface Vedirect {
    full_update: boolean;
    instances: { [key: string]: VedirectInstance };
}

export interface VedirectInstance {
    data_age_ms: number;
    device: VedirectDevice;
    output: VedirectOutput;
    input: VedirectInput;
}

export interface VedirectDevice {
    SER: string;
    PID: string;
    FW: string;
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
