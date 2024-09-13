import type { ValueObject } from '@/types/LiveDataStatus';

export interface StringValue {
    value: string;
    translate: boolean;
}

type BatteryData = (ValueObject | StringValue)[];

export interface Battery {
    manufacturer: string;
    serial: string;
    fwversion: string;
    hwversion: string;
    data_age: number;
    values: BatteryData[];
    showIssues: boolean;
    issues: number[];
}
