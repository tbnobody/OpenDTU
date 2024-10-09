export interface MqttConfig {
    mqtt_enabled: boolean;
    mqtt_hostname: string;
    mqtt_port: number;
    mqtt_clientid: string;
    mqtt_username: string;
    mqtt_password: string;
    mqtt_topic: string;
    mqtt_publish_interval: number;
    mqtt_clean_session: boolean;
    mqtt_retain: boolean;
    mqtt_tls: boolean;
    mqtt_root_ca_cert: string;
    mqtt_tls_cert_login: boolean;
    mqtt_client_cert: string;
    mqtt_client_key: string;
    mqtt_lwt_topic: string;
    mqtt_lwt_online: string;
    mqtt_lwt_offline: string;
    mqtt_lwt_qos: number;
    mqtt_hass_enabled: boolean;
    mqtt_hass_expire: boolean;
    mqtt_hass_retain: boolean;
    mqtt_hass_topic: string;
    mqtt_hass_individualpanels: boolean;
}
