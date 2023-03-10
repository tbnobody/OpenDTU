import type { ValueObject } from '@/types/LiveDataStatus';

// Ve.Direct 
export interface Vedirect {
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
    V: ValueObject;
    I: ValueObject;
    VPV: ValueObject;
    PPV: ValueObject;
    H19: ValueObject;
    H20: ValueObject;
    H21: ValueObject;
    H22: ValueObject;
    H23: ValueObject;
    PLSTATE: number;
    PLLIMIT: number;
}