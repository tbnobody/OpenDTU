import type { ValueObject } from '@/types/LiveDataStatus';

export interface DynamicPowerLimiter {
    PLSTATE: number;
    PLLIMIT: number;
}

export interface Vedirect {
    full_update: boolean;
    instances: { [key: string]: VedirectInstance };
}

type MpptData = (ValueObject | string)[];

export interface VedirectInstance {
    data_age_ms: number;
    product_id: string;
    firmware_version: string;
    values: { [key: string]: MpptData };
}
