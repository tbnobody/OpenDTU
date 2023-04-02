import type { ValueObject } from '@/types/LiveDataStatus';

interface BatteryFlags {
    dischargeCurrent: boolean;
    chargeCurrent: boolean;
    lowTemperature: boolean;
    highTemperature: boolean;
    lowVoltage: boolean;
    highVoltage: boolean;
    bmsInternal: boolean;
}


// Battery
export interface Battery {
    data_age: 0;
    chargeVoltage: ValueObject;
    chargeCurrentLimitation: ValueObject;
    dischargeCurrentLimitation: ValueObject;
    stateOfCharge: ValueObject;
    stateOfChargeLastUpdate: ValueObject;
    stateOfHealth: ValueObject;
    voltage: ValueObject;
    current: ValueObject;
    temperature: ValueObject;
    warnings: BatteryFlags;
    alarms: BatteryFlags;
    manufacturer: string;
    chargeEnabled: boolean;
    dischargeEnabled: boolean;
    chargeImmediately: boolean;
}