export interface PowerLimiterInverterInfo {
    serial: string;
    pos: number;
    order: number;
    name: string;
    poll_enable: boolean;
    poll_enable_night: boolean;
    command_enable: boolean;
    command_enable_night: boolean;
    max_power: number;
    type: string;
    channels: number;
}

// meta-data not directly part of the DPL settings,
// to control visibility of DPL settings
export interface PowerLimiterMetaData {
    power_meter_enabled: boolean;
    battery_enabled: boolean;
    charge_controller_enabled: boolean;
    inverters: PowerLimiterInverterInfo[];
}

export interface PowerLimiterInverterConfig {
    serial: string;
    is_governed: boolean;
    is_behind_power_meter: boolean;
    is_solar_powered: boolean;
    use_overscaling_to_compensate_shading: boolean;
    lower_power_limit: number;
    upper_power_limit: number;
}

export interface PowerLimiterConfig {
    enabled: boolean;
    verbose_logging: boolean;
    solar_passthrough_enabled: boolean;
    solar_passthrough_losses: number;
    battery_always_use_at_night: boolean;
    target_power_consumption: number;
    target_power_consumption_hysteresis: number;
    base_load_limit: number;
    ignore_soc: boolean;
    battery_soc_start_threshold: number;
    battery_soc_stop_threshold: number;
    voltage_start_threshold: number;
    voltage_stop_threshold: number;
    voltage_load_correction_factor: number;
    inverter_restart_hour: number;
    full_solar_passthrough_soc: number;
    full_solar_passthrough_start_voltage: number;
    full_solar_passthrough_stop_voltage: number;
    inverter_serial_for_dc_voltage: string;
    inverter_channel_id_for_dc_voltage: number;
    restart_hour: number;
    total_upper_power_limit: number;
    inverters: PowerLimiterInverterConfig[];
}
