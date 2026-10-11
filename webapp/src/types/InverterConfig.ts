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
    fragment_reply_timeout: number;
    fragment_retry_limit: number;
    effective_fragment_reply_timeout: number | null;
    effective_fragment_retry_limit: number | null;
    default_fragment_reply_timeout: number | null;
    default_fragment_retry_limit: number | null;
    zero_runtime: boolean;
    zero_day: boolean;
    clear_eventlog: boolean;
    yieldday_correction: boolean;
    yield_total_contribution: boolean;
    channel: Array<InverterChannel>;
}
