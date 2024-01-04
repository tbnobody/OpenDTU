export interface BatteryConfig {
    enabled: boolean;
    verbose_logging: boolean;
    provider: number;
    jkbms_interface: number;
    jkbms_polling_interval: number;
    mqtt_topic: string;
}
