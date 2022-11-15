export interface MqttStatus {
    mqtt_enabled: boolean;
    mqtt_hostname: string;
    mqtt_port: number;
    mqtt_username: string;
    mqtt_topic: string;
    mqtt_publish_interval: number;
    mqtt_retain: boolean;
    mqtt_tls: boolean;
    mqtt_root_ca_cert_info: string;
    mqtt_connected: boolean;
    mqtt_hass_enabled: boolean;
    mqtt_hass_expire: boolean;
    mqtt_hass_retain: boolean;
    mqtt_hass_topic: string;
    mqtt_hass_individualpanels: boolean;
}