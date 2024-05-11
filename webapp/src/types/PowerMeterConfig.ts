import type { HttpRequestConfig } from '@/types/HttpRequestConfig';

export interface PowerMeterHttpJsonConfig {
    index: number;
    http_request: HttpRequestConfig;
    enabled: boolean;
    json_path: string;
    unit: number;
    sign_inverted: boolean;
}

export interface PowerMeterHttpSmlConfig {
    http_request: HttpRequestConfig;
}

export interface PowerMeterConfig {
    enabled: boolean;
    verbose_logging: boolean;
    source: number;
    interval: number;
    mqtt_topic_powermeter_1: string;
    mqtt_topic_powermeter_2: string;
    mqtt_topic_powermeter_3: string;
    sdmaddress: number;
    http_individual_requests: boolean;
    http_json: Array<PowerMeterHttpJsonConfig>;
    http_sml: PowerMeterHttpSmlConfig;
}
