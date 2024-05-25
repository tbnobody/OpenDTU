export interface InverterChannel {
    name: string;
    max_power: number;
    yield_total_offset: number;
}

export interface Inverter {
    id: string;
    serial: string;
    name: string;
    type: string;
    order: number;
    poll_enable: boolean;
    poll_enable_night: boolean;
    command_enable: boolean;
    command_enable_night: boolean;
    reachable_threshold: number;
    zero_runtime: boolean;
    zero_day: boolean;
    yieldday_correction: boolean;
    channel: Array<InverterChannel>;
}
