// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_mqtt.h"
#include "Configuration.h"
#include "MqttHandleHass.h"
#include "MqttHandleInverter.h"
#include "MqttSettings.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "helper.h"
#include <AsyncJson.h>

void WebApiMqttClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/mqtt/status", HTTP_GET, std::bind(&WebApiMqttClass::onMqttStatus, this, _1));
    server.on("/api/mqtt/config", HTTP_GET, std::bind(&WebApiMqttClass::onMqttAdminGet, this, _1));
    server.on("/api/mqtt/config", HTTP_POST, std::bind(&WebApiMqttClass::onMqttAdminPost, this, _1));
}

void WebApiMqttClass::onMqttStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["mqtt_enabled"] = config.Mqtt.Enabled;
    root["mqtt_hostname"] = config.Mqtt.Hostname;
    root["mqtt_port"] = config.Mqtt.Port;
    root["mqtt_username"] = config.Mqtt.Username;
    root["mqtt_topic"] = config.Mqtt.Topic;
    root["mqtt_connected"] = MqttSettings.getConnected();
    root["mqtt_retain"] = config.Mqtt.Retain;
    root["mqtt_tls"] = config.Mqtt.Tls.Enabled;
    root["mqtt_root_ca_cert_info"] = getTlsCertInfo(config.Mqtt.Tls.RootCaCert);
    root["mqtt_tls_cert_login"] = config.Mqtt.Tls.CertLogin;
    root["mqtt_client_cert_info"] = getTlsCertInfo(config.Mqtt.Tls.ClientCert);
    root["mqtt_lwt_topic"] = String(config.Mqtt.Topic) + config.Mqtt.Lwt.Topic;
    root["mqtt_publish_interval"] = config.Mqtt.PublishInterval;
    root["mqtt_clean_session"] = config.Mqtt.CleanSession;
    root["mqtt_hass_enabled"] = config.Mqtt.Hass.Enabled;
    root["mqtt_hass_expire"] = config.Mqtt.Hass.Expire;
    root["mqtt_hass_retain"] = config.Mqtt.Hass.Retain;
    root["mqtt_hass_topic"] = config.Mqtt.Hass.Topic;
    root["mqtt_hass_individualpanels"] = config.Mqtt.Hass.IndividualPanels;

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiMqttClass::onMqttAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["mqtt_enabled"] = config.Mqtt.Enabled;
    root["mqtt_hostname"] = config.Mqtt.Hostname;
    root["mqtt_port"] = config.Mqtt.Port;
    root["mqtt_username"] = config.Mqtt.Username;
    root["mqtt_password"] = config.Mqtt.Password;
    root["mqtt_topic"] = config.Mqtt.Topic;
    root["mqtt_retain"] = config.Mqtt.Retain;
    root["mqtt_tls"] = config.Mqtt.Tls.Enabled;
    root["mqtt_root_ca_cert"] = config.Mqtt.Tls.RootCaCert;
    root["mqtt_tls_cert_login"] = config.Mqtt.Tls.CertLogin;
    root["mqtt_client_cert"] = config.Mqtt.Tls.ClientCert;
    root["mqtt_client_key"] = config.Mqtt.Tls.ClientKey;
    root["mqtt_lwt_topic"] = config.Mqtt.Lwt.Topic;
    root["mqtt_lwt_online"] = config.Mqtt.Lwt.Value_Online;
    root["mqtt_lwt_offline"] = config.Mqtt.Lwt.Value_Offline;
    root["mqtt_lwt_qos"] = config.Mqtt.Lwt.Qos;
    root["mqtt_publish_interval"] = config.Mqtt.PublishInterval;
    root["mqtt_clean_session"] = config.Mqtt.CleanSession;
    root["mqtt_hass_enabled"] = config.Mqtt.Hass.Enabled;
    root["mqtt_hass_expire"] = config.Mqtt.Hass.Expire;
    root["mqtt_hass_retain"] = config.Mqtt.Hass.Retain;
    root["mqtt_hass_topic"] = config.Mqtt.Hass.Topic;
    root["mqtt_hass_individualpanels"] = config.Mqtt.Hass.IndividualPanels;

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiMqttClass::onMqttAdminPost(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonDocument root;
    if (!WebApi.parseRequestData(request, response, root)) {
        return;
    }

    auto& retMsg = response->getRoot();

    if (!(root.containsKey("mqtt_enabled")
            && root.containsKey("mqtt_hostname")
            && root.containsKey("mqtt_port")
            && root.containsKey("mqtt_username")
            && root.containsKey("mqtt_password")
            && root.containsKey("mqtt_topic")
            && root.containsKey("mqtt_retain")
            && root.containsKey("mqtt_tls")
            && root.containsKey("mqtt_tls_cert_login")
            && root.containsKey("mqtt_client_cert")
            && root.containsKey("mqtt_client_key")
            && root.containsKey("mqtt_lwt_topic")
            && root.containsKey("mqtt_lwt_online")
            && root.containsKey("mqtt_lwt_offline")
            && root.containsKey("mqtt_lwt_qos")
            && root.containsKey("mqtt_publish_interval")
            && root.containsKey("mqtt_clean_session")
            && root.containsKey("mqtt_hass_enabled")
            && root.containsKey("mqtt_hass_expire")
            && root.containsKey("mqtt_hass_retain")
            && root.containsKey("mqtt_hass_topic")
            && root.containsKey("mqtt_hass_individualpanels"))) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["mqtt_enabled"].as<bool>()) {
        if (root["mqtt_hostname"].as<String>().length() == 0 || root["mqtt_hostname"].as<String>().length() > MQTT_MAX_HOSTNAME_STRLEN) {
            retMsg["message"] = "MqTT Server must between 1 and " STR(MQTT_MAX_HOSTNAME_STRLEN) " characters long!";
            retMsg["code"] = WebApiError::MqttHostnameLength;
            retMsg["param"]["max"] = MQTT_MAX_HOSTNAME_STRLEN;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }

        if (root["mqtt_username"].as<String>().length() > MQTT_MAX_USERNAME_STRLEN) {
            retMsg["message"] = "Username must not be longer than " STR(MQTT_MAX_USERNAME_STRLEN) " characters!";
            retMsg["code"] = WebApiError::MqttUsernameLength;
            retMsg["param"]["max"] = MQTT_MAX_USERNAME_STRLEN;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }
        if (root["mqtt_password"].as<String>().length() > MQTT_MAX_PASSWORD_STRLEN) {
            retMsg["message"] = "Password must not be longer than " STR(MQTT_MAX_PASSWORD_STRLEN) " characters!";
            retMsg["code"] = WebApiError::MqttPasswordLength;
            retMsg["param"]["max"] = MQTT_MAX_PASSWORD_STRLEN;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }
        if (root["mqtt_topic"].as<String>().length() > MQTT_MAX_TOPIC_STRLEN) {
            retMsg["message"] = "Topic must not be longer than " STR(MQTT_MAX_TOPIC_STRLEN) " characters!";
            retMsg["code"] = WebApiError::MqttTopicLength;
            retMsg["param"]["max"] = MQTT_MAX_TOPIC_STRLEN;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }

        if (root["mqtt_topic"].as<String>().indexOf(' ') != -1) {
            retMsg["message"] = "Topic must not contain space characters!";
            retMsg["code"] = WebApiError::MqttTopicCharacter;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }

        if (!root["mqtt_topic"].as<String>().endsWith("/")) {
            retMsg["message"] = "Topic must end with a slash (/)!";
            retMsg["code"] = WebApiError::MqttTopicTrailingSlash;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }

        if (root["mqtt_port"].as<uint>() == 0 || root["mqtt_port"].as<uint>() > 65535) {
            retMsg["message"] = "Port must be a number between 1 and 65535!";
            retMsg["code"] = WebApiError::MqttPort;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }

        if (root["mqtt_root_ca_cert"].as<String>().length() > MQTT_MAX_CERT_STRLEN
            || root["mqtt_client_cert"].as<String>().length() > MQTT_MAX_CERT_STRLEN
            || root["mqtt_client_key"].as<String>().length() > MQTT_MAX_CERT_STRLEN) {
            retMsg["message"] = "Certificates must not be longer than " STR(MQTT_MAX_CERT_STRLEN) " characters!";
            retMsg["code"] = WebApiError::MqttCertificateLength;
            retMsg["param"]["max"] = MQTT_MAX_CERT_STRLEN;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }

        if (root["mqtt_lwt_topic"].as<String>().length() > MQTT_MAX_TOPIC_STRLEN) {
            retMsg["message"] = "LWT topic must not be longer than " STR(MQTT_MAX_TOPIC_STRLEN) " characters!";
            retMsg["code"] = WebApiError::MqttLwtTopicLength;
            retMsg["param"]["max"] = MQTT_MAX_TOPIC_STRLEN;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }

        if (root["mqtt_lwt_topic"].as<String>().indexOf(' ') != -1) {
            retMsg["message"] = "LWT topic must not contain space characters!";
            retMsg["code"] = WebApiError::MqttLwtTopicCharacter;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }

        if (root["mqtt_lwt_online"].as<String>().length() > MQTT_MAX_LWTVALUE_STRLEN) {
            retMsg["message"] = "LWT online value must not be longer than " STR(MQTT_MAX_LWTVALUE_STRLEN) " characters!";
            retMsg["code"] = WebApiError::MqttLwtOnlineLength;
            retMsg["param"]["max"] = MQTT_MAX_LWTVALUE_STRLEN;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }

        if (root["mqtt_lwt_offline"].as<String>().length() > MQTT_MAX_LWTVALUE_STRLEN) {
            retMsg["message"] = "LWT offline value must not be longer than " STR(MQTT_MAX_LWTVALUE_STRLEN) " characters!";
            retMsg["code"] = WebApiError::MqttLwtOfflineLength;
            retMsg["param"]["max"] = MQTT_MAX_LWTVALUE_STRLEN;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }

        if (root["mqtt_lwt_qos"].as<uint8_t>() > 2) {
            retMsg["message"] = "LWT QoS must not be greater than " STR(2) "!";
            retMsg["code"] = WebApiError::MqttLwtQos;
            retMsg["param"]["max"] = 2;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }

        if (root["mqtt_publish_interval"].as<uint32_t>() < 5 || root["mqtt_publish_interval"].as<uint32_t>() > 65535) {
            retMsg["message"] = "Publish interval must be a number between 5 and 65535!";
            retMsg["code"] = WebApiError::MqttPublishInterval;
            retMsg["param"]["min"] = 5;
            retMsg["param"]["max"] = 65535;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }

        if (root["mqtt_hass_enabled"].as<bool>()) {
            if (root["mqtt_hass_topic"].as<String>().length() > MQTT_MAX_TOPIC_STRLEN) {
                retMsg["message"] = "Hass topic must not be longer than " STR(MQTT_MAX_TOPIC_STRLEN) " characters!";
                retMsg["code"] = WebApiError::MqttHassTopicLength;
                retMsg["param"]["max"] = MQTT_MAX_TOPIC_STRLEN;
                WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
                return;
            }

            if (root["mqtt_hass_topic"].as<String>().indexOf(' ') != -1) {
                retMsg["message"] = "Hass topic must not contain space characters!";
                retMsg["code"] = WebApiError::MqttHassTopicCharacter;
                WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
                return;
            }
        }
    }

    CONFIG_T& config = Configuration.get();
    config.Mqtt.Enabled = root["mqtt_enabled"].as<bool>();
    config.Mqtt.Retain = root["mqtt_retain"].as<bool>();
    config.Mqtt.Tls.Enabled = root["mqtt_tls"].as<bool>();
    strlcpy(config.Mqtt.Tls.RootCaCert, root["mqtt_root_ca_cert"].as<String>().c_str(), sizeof(config.Mqtt.Tls.RootCaCert));
    config.Mqtt.Tls.CertLogin = root["mqtt_tls_cert_login"].as<bool>();
    strlcpy(config.Mqtt.Tls.ClientCert, root["mqtt_client_cert"].as<String>().c_str(), sizeof(config.Mqtt.Tls.ClientCert));
    strlcpy(config.Mqtt.Tls.ClientKey, root["mqtt_client_key"].as<String>().c_str(), sizeof(config.Mqtt.Tls.ClientKey));
    config.Mqtt.Port = root["mqtt_port"].as<uint>();
    strlcpy(config.Mqtt.Hostname, root["mqtt_hostname"].as<String>().c_str(), sizeof(config.Mqtt.Hostname));
    strlcpy(config.Mqtt.Username, root["mqtt_username"].as<String>().c_str(), sizeof(config.Mqtt.Username));
    strlcpy(config.Mqtt.Password, root["mqtt_password"].as<String>().c_str(), sizeof(config.Mqtt.Password));
    strlcpy(config.Mqtt.Lwt.Topic, root["mqtt_lwt_topic"].as<String>().c_str(), sizeof(config.Mqtt.Lwt.Topic));
    strlcpy(config.Mqtt.Lwt.Value_Online, root["mqtt_lwt_online"].as<String>().c_str(), sizeof(config.Mqtt.Lwt.Value_Online));
    strlcpy(config.Mqtt.Lwt.Value_Offline, root["mqtt_lwt_offline"].as<String>().c_str(), sizeof(config.Mqtt.Lwt.Value_Offline));
    config.Mqtt.Lwt.Qos = root["mqtt_lwt_qos"].as<uint8_t>();
    config.Mqtt.PublishInterval = root["mqtt_publish_interval"].as<uint32_t>();
    config.Mqtt.CleanSession = root["mqtt_clean_session"].as<bool>();
    config.Mqtt.Hass.Enabled = root["mqtt_hass_enabled"].as<bool>();
    config.Mqtt.Hass.Expire = root["mqtt_hass_expire"].as<bool>();
    config.Mqtt.Hass.Retain = root["mqtt_hass_retain"].as<bool>();
    config.Mqtt.Hass.IndividualPanels = root["mqtt_hass_individualpanels"].as<bool>();
    strlcpy(config.Mqtt.Hass.Topic, root["mqtt_hass_topic"].as<String>().c_str(), sizeof(config.Mqtt.Hass.Topic));

    // Check if base topic was changed
    if (strcmp(config.Mqtt.Topic, root["mqtt_topic"].as<String>().c_str())) {
        MqttHandleInverter.unsubscribeTopics();
        strlcpy(config.Mqtt.Topic, root["mqtt_topic"].as<String>().c_str(), sizeof(config.Mqtt.Topic));
        MqttHandleInverter.subscribeTopics();
    }

    WebApi.writeConfig(retMsg);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    MqttSettings.performReconnect();
    MqttHandleHass.forceUpdate();
}

String WebApiMqttClass::getTlsCertInfo(const char* cert)
{
    char tlsCertInfo[1024] = "";

    mbedtls_x509_crt tlsCert;

    strlcpy(tlsCertInfo, "Can't parse TLS certificate", sizeof(tlsCertInfo));

    mbedtls_x509_crt_init(&tlsCert);
    int ret = mbedtls_x509_crt_parse(&tlsCert, const_cast<unsigned char*>((unsigned char*)cert), 1 + strlen(cert));
    if (ret < 0) {
        snprintf(tlsCertInfo, sizeof(tlsCertInfo), "Can't parse TLS certificate: mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
        mbedtls_x509_crt_free(&tlsCert);
        return "";
    }
    mbedtls_x509_crt_info(tlsCertInfo, sizeof(tlsCertInfo) - 1, "", &tlsCert);
    mbedtls_x509_crt_free(&tlsCert);

    return tlsCertInfo;
}
