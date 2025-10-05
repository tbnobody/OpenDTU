import type { ValueObject } from '@/types/LiveDataStatus';
import type { StringValue } from '@/types/StringValue';

type DataPoint = (ValueObject | StringValue)[];

export interface GridCharger {
    dataAge: number;
    provider: number;
    reachable: boolean;
    producing: boolean;
    serial?: string;
    vendorName?: string;
    productName?: string;
    values?: { [key: string]: DataPoint };
}
