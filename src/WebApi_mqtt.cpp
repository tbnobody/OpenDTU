// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_mqtt.h"
#include "Configuration.h"
#include "MqttHandleHass.h"
#include "MqttSettings.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "helper.h"
#include <AsyncJson.h>

void WebApiMqttClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/api/mqtt/status", HTTP_GET, std::bind(&WebApiMqttClass::onMqttStatus, this, _1));
    _server->on("/api/mqtt/config", HTTP_GET, std::bind(&WebApiMqttClass::onMqttAdminGet, this, _1));
    _server->on("/api/mqtt/config", HTTP_POST, std::bind(&WebApiMqttClass::onMqttAdminPost, this, _1));
}

void WebApiMqttClass::loop()
{
}

void WebApiMqttClass::onMqttStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse(false, MQTT_JSON_DOC_SIZE);
    JsonObject root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["mqtt_enabled"] = config.Mqtt_Enabled;
    root["mqtt_hostname"] = config.Mqtt_Hostname;
    root["mqtt_port"] = config.Mqtt_Port;
    root["mqtt_username"] = config.Mqtt_Username;
    root["mqtt_topic"] = config.Mqtt_Topic;
    root["mqtt_connected"] = MqttSettings.getConnected();
    root["mqtt_retain"] = config.Mqtt_Retain;
    root["mqtt_tls"] = config.Mqtt_Tls;
    root["mqtt_root_ca_cert_info"] = getTlsCertInfo(config.Mqtt_RootCaCert);
    root["mqtt_tls_cert_login"] = config.Mqtt_TlsCertLogin;
    root["mqtt_client_cert_info"] = getTlsCertInfo(config.Mqtt_ClientCert);
    root["mqtt_lwt_topic"] = String(config.Mqtt_Topic) + config.Mqtt_LwtTopic;
    root["mqtt_publish_interval"] = config.Mqtt_PublishInterval;
    root["mqtt_hass_enabled"] = config.Mqtt_Hass_Enabled;
    root["mqtt_hass_expire"] = config.Mqtt_Hass_Expire;
    root["mqtt_hass_retain"] = config.Mqtt_Hass_Retain;
    root["mqtt_hass_topic"] = config.Mqtt_Hass_Topic;
    root["mqtt_hass_individualpanels"] = config.Mqtt_Hass_IndividualPanels;

    response->setLength();
    request->send(response);
}

void WebApiMqttClass::onMqttAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse(false, MQTT_JSON_DOC_SIZE);
    JsonObject root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["mqtt_enabled"] = config.Mqtt_Enabled;
    root["mqtt_hostname"] = config.Mqtt_Hostname;
    root["mqtt_port"] = config.Mqtt_Port;
    root["mqtt_username"] = config.Mqtt_Username;
    root["mqtt_password"] = config.Mqtt_Password;
    root["mqtt_topic"] = config.Mqtt_Topic;
    root["mqtt_retain"] = config.Mqtt_Retain;
    root["mqtt_tls"] = config.Mqtt_Tls;
    root["mqtt_root_ca_cert"] = config.Mqtt_RootCaCert;
    root["mqtt_tls_cert_login"] = config.Mqtt_TlsCertLogin;
    root["mqtt_client_cert"] = config.Mqtt_ClientCert;
    root["mqtt_client_key"] = config.Mqtt_ClientKey;
    root["mqtt_lwt_topic"] = config.Mqtt_LwtTopic;
    root["mqtt_lwt_online"] = config.Mqtt_LwtValue_Online;
    root["mqtt_lwt_offline"] = config.Mqtt_LwtValue_Offline;
    root["mqtt_publish_interval"] = config.Mqtt_PublishInterval;
    root["mqtt_hass_enabled"] = config.Mqtt_Hass_Enabled;
    root["mqtt_hass_expire"] = config.Mqtt_Hass_Expire;
    root["mqtt_hass_retain"] = config.Mqtt_Hass_Retain;
    root["mqtt_hass_topic"] = config.Mqtt_Hass_Topic;
    root["mqtt_hass_individualpanels"] = config.Mqtt_Hass_IndividualPanels;

    response->setLength();
    request->send(response);
}

void WebApiMqttClass::onMqttAdminPost(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse(false, MQTT_JSON_DOC_SIZE);
    JsonObject retMsg = response->getRoot();
    retMsg["type"] = "warning";

    if (!request->hasParam("data", true)) {
        retMsg["message"] = "No values found!";
        retMsg["code"] = WebApiError::GenericNoValueFound;
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > MQTT_JSON_DOC_SIZE) {
        retMsg["message"] = "Data too large!";
        retMsg["code"] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(MQTT_JSON_DOC_SIZE);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg["message"] = "Failed to parse data!";
        retMsg["code"] = WebApiError::GenericParseError;
        response->setLength();
        request->send(response);
        return;
    }

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
            && root.containsKey("mqtt_publish_interval")
            && root.containsKey("mqtt_hass_enabled")
            && root.containsKey("mqtt_hass_expire")
            && root.containsKey("mqtt_hass_retain")
            && root.containsKey("mqtt_hass_topic")
            && root.containsKey("mqtt_hass_individualpanels"))) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["mqtt_enabled"].as<bool>()) {
        if (root["mqtt_hostname"].as<String>().length() == 0 || root["mqtt_hostname"].as<String>().length() > MQTT_MAX_HOSTNAME_STRLEN) {
            retMsg["message"] = "MqTT Server must between 1 and " STR(MQTT_MAX_HOSTNAME_STRLEN) " characters long!";
            retMsg["code"] = WebApiError::MqttHostnameLength;
            retMsg["param"]["max"] = MQTT_MAX_HOSTNAME_STRLEN;
            response->setLength();
            request->send(response);
            return;
        }

        if (root["mqtt_username"].as<String>().length() > MQTT_MAX_USERNAME_STRLEN) {
            retMsg["message"] = "Username must not be longer than " STR(MQTT_MAX_USERNAME_STRLEN) " characters!";
            retMsg["code"] = WebApiError::MqttUsernameLength;
            retMsg["param"]["max"] = MQTT_MAX_USERNAME_STRLEN;
            response->setLength();
            request->send(response);
            return;
        }
        if (root["mqtt_password"].as<String>().length() > MQTT_MAX_PASSWORD_STRLEN) {
            retMsg["message"] = "Password must not be longer than " STR(MQTT_MAX_PASSWORD_STRLEN) " characters!";
            retMsg["code"] = WebApiError::MqttPasswordLength;
            retMsg["param"]["max"] = MQTT_MAX_PASSWORD_STRLEN;
            response->setLength();
            request->send(response);
            return;
        }
        if (root["mqtt_topic"].as<String>().length() > MQTT_MAX_TOPIC_STRLEN) {
            retMsg["message"] = "Topic must not be longer than " STR(MQTT_MAX_TOPIC_STRLEN) " characters!";
            retMsg["code"] = WebApiError::MqttTopicLength;
            retMsg["param"]["max"] = MQTT_MAX_TOPIC_STRLEN;
            response->setLength();
            request->send(response);
            return;
        }

        if (root["mqtt_topic"].as<String>().indexOf(' ') != -1) {
            retMsg["message"] = "Topic must not contain space characters!";
            retMsg["code"] = WebApiError::MqttTopicCharacter;
            response->setLength();
            request->send(response);
            return;
        }

        if (!root["mqtt_topic"].as<String>().endsWith("/")) {
            retMsg["message"] = "Topic must end with a slash (/)!";
            retMsg["code"] = WebApiError::MqttTopicTrailingSlash;
            response->setLength();
            request->send(response);
            return;
        }

        if (root["mqtt_port"].as<uint>() == 0 || root["mqtt_port"].as<uint>() > 65535) {
            retMsg["message"] = "Port must be a number between 1 and 65535!";
            retMsg["code"] = WebApiError::MqttPort;
            response->setLength();
            request->send(response);
            return;
        }

        if (root["mqtt_root_ca_cert"].as<String>().length() > MQTT_MAX_CERT_STRLEN
            || root["mqtt_client_cert"].as<String>().length() > MQTT_MAX_CERT_STRLEN
            || root["mqtt_client_key"].as<String>().length() > MQTT_MAX_CERT_STRLEN) {
            retMsg["message"] = "Certificates must not be longer than " STR(MQTT_MAX_CERT_STRLEN) " characters!";
            retMsg["code"] = WebApiError::MqttCertificateLength;
            retMsg["param"]["max"] = MQTT_MAX_CERT_STRLEN;
            response->setLength();
            request->send(response);
            return;
        }

        if (root["mqtt_lwt_topic"].as<String>().length() > MQTT_MAX_TOPIC_STRLEN) {
            retMsg["message"] = "LWT topic must not be longer than " STR(MQTT_MAX_TOPIC_STRLEN) " characters!";
            retMsg["code"] = WebApiError::MqttLwtTopicLength;
            retMsg["param"]["max"] = MQTT_MAX_TOPIC_STRLEN;
            response->setLength();
            request->send(response);
            return;
        }

        if (root["mqtt_lwt_topic"].as<String>().indexOf(' ') != -1) {
            retMsg["message"] = "LWT topic must not contain space characters!";
            retMsg["code"] = WebApiError::MqttLwtTopicCharacter;
            response->setLength();
            request->send(response);
            return;
        }

        if (root["mqtt_lwt_online"].as<String>().length() > MQTT_MAX_LWTVALUE_STRLEN) {
            retMsg["message"] = "LWT online value must not be longer than " STR(MQTT_MAX_LWTVALUE_STRLEN) " characters!";
            retMsg["code"] = WebApiError::MqttLwtOnlineLength;
            retMsg["param"]["max"] = MQTT_MAX_LWTVALUE_STRLEN;
            response->setLength();
            request->send(response);
            return;
        }

        if (root["mqtt_lwt_offline"].as<String>().length() > MQTT_MAX_LWTVALUE_STRLEN) {
            retMsg["message"] = "LWT offline value must not be longer than " STR(MQTT_MAX_LWTVALUE_STRLEN) " characters!";
            retMsg["code"] = WebApiError::MqttLwtOfflineLength;
            retMsg["param"]["max"] = MQTT_MAX_LWTVALUE_STRLEN;
            response->setLength();
            request->send(response);
            return;
        }

        if (root["mqtt_publish_interval"].as<uint32_t>() < 5 || root["mqtt_publish_interval"].as<uint32_t>() > 65535) {
            retMsg["message"] = "Publish interval must be a number between 5 and 65535!";
            retMsg["code"] = WebApiError::MqttPublishInterval;
            retMsg["param"]["min"] = 5;
            retMsg["param"]["max"] = 65535;
            response->setLength();
            request->send(response);
            return;
        }

        if (root["mqtt_hass_enabled"].as<bool>()) {
            if (root["mqtt_hass_topic"].as<String>().length() > MQTT_MAX_TOPIC_STRLEN) {
                retMsg["message"] = "Hass topic must not be longer than " STR(MQTT_MAX_TOPIC_STRLEN) " characters!";
                retMsg["code"] = WebApiError::MqttHassTopicLength;
                retMsg["param"]["max"] = MQTT_MAX_TOPIC_STRLEN;
                response->setLength();
                request->send(response);
                return;
            }

            if (root["mqtt_hass_topic"].as<String>().indexOf(' ') != -1) {
                retMsg["message"] = "Hass topic must not contain space characters!";
                retMsg["code"] = WebApiError::MqttHassTopicCharacter;
                response->setLength();
                request->send(response);
                return;
            }
        }
    }

    CONFIG_T& config = Configuration.get();
    config.Mqtt_Enabled = root["mqtt_enabled"].as<bool>();
    config.Mqtt_Retain = root["mqtt_retain"].as<bool>();
    config.Mqtt_Tls = root["mqtt_tls"].as<bool>();
    strlcpy(config.Mqtt_RootCaCert, root["mqtt_root_ca_cert"].as<String>().c_str(), sizeof(config.Mqtt_RootCaCert));
    config.Mqtt_TlsCertLogin = root["mqtt_tls_cert_login"].as<bool>();
    strlcpy(config.Mqtt_ClientCert, root["mqtt_client_cert"].as<String>().c_str(), sizeof(config.Mqtt_ClientCert));
    strlcpy(config.Mqtt_ClientKey, root["mqtt_client_key"].as<String>().c_str(), sizeof(config.Mqtt_ClientKey));
    config.Mqtt_Port = root["mqtt_port"].as<uint>();
    strlcpy(config.Mqtt_Hostname, root["mqtt_hostname"].as<String>().c_str(), sizeof(config.Mqtt_Hostname));
    strlcpy(config.Mqtt_Username, root["mqtt_username"].as<String>().c_str(), sizeof(config.Mqtt_Username));
    strlcpy(config.Mqtt_Password, root["mqtt_password"].as<String>().c_str(), sizeof(config.Mqtt_Password));
    strlcpy(config.Mqtt_Topic, root["mqtt_topic"].as<String>().c_str(), sizeof(config.Mqtt_Topic));
    strlcpy(config.Mqtt_LwtTopic, root["mqtt_lwt_topic"].as<String>().c_str(), sizeof(config.Mqtt_LwtTopic));
    strlcpy(config.Mqtt_LwtValue_Online, root["mqtt_lwt_online"].as<String>().c_str(), sizeof(config.Mqtt_LwtValue_Online));
    strlcpy(config.Mqtt_LwtValue_Offline, root["mqtt_lwt_offline"].as<String>().c_str(), sizeof(config.Mqtt_LwtValue_Offline));
    config.Mqtt_PublishInterval = root["mqtt_publish_interval"].as<uint32_t>();
    config.Mqtt_Hass_Enabled = root["mqtt_hass_enabled"].as<bool>();
    config.Mqtt_Hass_Expire = root["mqtt_hass_expire"].as<bool>();
    config.Mqtt_Hass_Retain = root["mqtt_hass_retain"].as<bool>();
    config.Mqtt_Hass_IndividualPanels = root["mqtt_hass_individualpanels"].as<bool>();
    strlcpy(config.Mqtt_Hass_Topic, root["mqtt_hass_topic"].as<String>().c_str(), sizeof(config.Mqtt_Hass_Topic));
    Configuration.write();

    retMsg["type"] = "success";
    retMsg["message"] = "Settings saved!";
    retMsg["code"] = WebApiError::GenericSuccess;

    response->setLength();
    request->send(response);

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
