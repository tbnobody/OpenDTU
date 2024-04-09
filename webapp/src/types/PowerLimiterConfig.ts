export interface PowerLimiterInverterInfo {
    pos: number;
    name: string;
    poll_enable: boolean;
    poll_enable_night: boolean;
    command_enable: boolean;
    command_enable_night: boolean;
    type: string;
    channels: number;
}

// meta-data not directly part of the DPL settings,
// to control visibility of DPL settings
export interface PowerLimiterMetaData {
    power_meter_enabled: boolean;
    battery_enabled: boolean;
    charge_controller_enabled: boolean;
    inverters: { [key: string]: PowerLimiterInverterInfo };
}

export interface PowerLimiterConfig {
    enabled: boolean;
    verbose_logging: boolean;
    solar_passthrough_enabled: boolean;
    solar_passthrough_losses: number;
    battery_always_use_at_night: boolean;
    is_inverter_behind_powermeter: boolean;
    is_inverter_solar_powered: boolean;
    inverter_serial: string;
    inverter_channel_id: number;
    target_power_consumption: number;
    target_power_consumption_hysteresis: number;
    lower_power_limit: number;
    base_load_limit: number;
    upper_power_limit: number;
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
}
