export interface PowerLimiterConfig {
    enabled: boolean;
    solar_passtrough_enabled: boolean;
    battery_drain_strategy: number;
    is_inverter_behind_powermeter: boolean;
    inverter_id: number;
    inverter_channel_id: number;
    target_power_consumption: number;
    target_power_consumption_hysteresis: number;
    lower_power_limit: number;
    upper_power_limit: number;
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
