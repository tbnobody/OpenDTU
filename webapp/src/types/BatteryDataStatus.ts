import type { ValueObject } from '@/types/LiveDataStatus';

type BatteryData = (ValueObject | string)[];

export interface Battery {
    manufacturer: string;
    fwversion: string;
    hwversion: string;
    data_age: number;
    values: BatteryData[];
    issues: number[];
}