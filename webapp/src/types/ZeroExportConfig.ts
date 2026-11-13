export interface ZeroExportConfig {
    enabled: boolean;
    setpoint: number;
    inverter_minimal_power: number;
    home_minimal_consumption: number;
    update_interval: number;
    source: number;
    shelly_lnm_addr: string;
    shelly_lnm_port: number;
    shelly_lnm_type: string;
    shelly_lnm_failsafe_timeout: number;
    mqtt_grid_power_topic: string;
    mqtt_data_type: string;
    mqtt_failsafe_timeout: number;
}

export interface ZeroExportInverterStatus {
    pos: number;
    serial: string;
    name: string;
    limit: number;
    startup_pending: boolean;
    startup_at_full_power: boolean;
    release_pending: boolean;
    failsafe_pending: boolean;
    reachable: boolean;
    commands_enabled: boolean;
}

export interface ZeroExportStatus {
    enabled: boolean;
    setpoint: number;
    grid_source_running: boolean;
    grid_power: number | null;
    grid_power_age: number;
    grid_power_failsafe: number;
    grid_update_period: number;
    production_limit: number;
    failsafe: boolean;
    inverters: ZeroExportInverterStatus[];
}
