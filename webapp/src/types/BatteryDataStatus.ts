import type { ValueObject } from '@/types/LiveDataStatus';

export interface Battery {
    manufacturer: string;
    data_age: number;
    values: (ValueObject | string)[];
    issues: number[];
}