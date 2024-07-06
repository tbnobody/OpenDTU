export interface DataLoggerOutputConfig {
    total_yield_total: boolean;
    total_yield_day: boolean;
    total_power: boolean;
}

export interface DataLoggerConfig {
    datalogger_enabled: boolean;
    saveinterval: number;
    filename: string;
    output_config: DataLoggerOutputConfig;
}
