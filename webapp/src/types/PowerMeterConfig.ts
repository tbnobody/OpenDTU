export interface PowerMeterHttpPhaseConfig {
    index: number;
    enabled: boolean;
    url: string;
    auth_type: number;
    username: string;
    password: string;
    header_key: string;
    header_value: string;
    json_path: string;
    timeout: number;
    unit: number;
    sign_inverted: boolean;
}

export interface PowerMeterConfig {
    enabled: boolean;
    verbose_logging: boolean;
    source: number;
    interval: number;
    mqtt_topic_powermeter_1: string;
    mqtt_topic_powermeter_2: string;
    mqtt_topic_powermeter_3: string;
    sdmbaudrate: number;
    sdmaddress: number;
    http_individual_requests: boolean;
    http_phases: Array<PowerMeterHttpPhaseConfig>;
}
