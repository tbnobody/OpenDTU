export interface BatteryZendureConfig {
    device_type: number;
    device_id: string;
    polling_interval: number;
    soc_min: number;
    soc_max: number;
    bypass_mode: number;
    max_output: number;
    auto_shutdown: boolean;
    output_limit: number;
    output_control: number;
    output_limit_day: number;
    output_limit_night: number;
    sunrise_offset: number;
    sunset_offset: number;
    charge_through_enable: boolean;
    charge_through_interval: number;
    buzzer_enable: boolean;
    control_mode: number;
    charge_through_reset: number;
}

export interface BatteryMqttConfig {
    soc_topic: string;
    soc_json_path: string;
    voltage_topic: string;
    voltage_json_path: string;
    voltage_unit: number;
    current_topic: string;
    current_json_path: string;
    current_unit: number;
    discharge_current_limit_topic: string;
    discharge_current_limit_json_path: string;
    discharge_current_limit_unit: number;
}

export interface BatterySerialConfig {
    interface: number;
    polling_interval: number;
}

export interface BatteryConfig {
    enabled: boolean;
    provider: number;
    serial: BatterySerialConfig;
    mqtt: BatteryMqttConfig;
    zendure: BatteryZendureConfig;
    enable_discharge_current_limit: boolean;
    discharge_current_limit: number;
    discharge_current_limit_below_soc: number;
    discharge_current_limit_below_voltage: number;
    use_battery_reported_discharge_current_limit: boolean;
}
