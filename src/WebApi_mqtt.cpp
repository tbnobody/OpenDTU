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

    root[F("mqtt_enabled")] = config.Mqtt_Enabled;
    root[F("mqtt_hostname")] = config.Mqtt_Hostname;
    root[F("mqtt_port")] = config.Mqtt_Port;
    root[F("mqtt_username")] = config.Mqtt_Username;
    root[F("mqtt_topic")] = config.Mqtt_Topic;
    root[F("mqtt_connected")] = MqttSettings.getConnected();
    root[F("mqtt_retain")] = config.Mqtt_Retain;
    root[F("mqtt_tls")] = config.Mqtt_Tls;
    root[F("mqtt_root_ca_cert_info")] = getRootCaCertInfo(config.Mqtt_RootCaCert);
    root[F("mqtt_lwt_topic")] = String(config.Mqtt_Topic) + config.Mqtt_LwtTopic;
    root[F("mqtt_publish_interval")] = config.Mqtt_PublishInterval;
    root[F("mqtt_hass_enabled")] = config.Mqtt_Hass_Enabled;
    root[F("mqtt_hass_expire")] = config.Mqtt_Hass_Expire;
    root[F("mqtt_hass_retain")] = config.Mqtt_Hass_Retain;
    root[F("mqtt_hass_topic")] = config.Mqtt_Hass_Topic;
    root[F("mqtt_hass_individualpanels")] = config.Mqtt_Hass_IndividualPanels;

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

    root[F("mqtt_enabled")] = config.Mqtt_Enabled;
    root[F("mqtt_hostname")] = config.Mqtt_Hostname;
    root[F("mqtt_port")] = config.Mqtt_Port;
    root[F("mqtt_username")] = config.Mqtt_Username;
    root[F("mqtt_password")] = config.Mqtt_Password;
    root[F("mqtt_topic")] = config.Mqtt_Topic;
    root[F("mqtt_retain")] = config.Mqtt_Retain;
    root[F("mqtt_tls")] = config.Mqtt_Tls;
    root[F("mqtt_root_ca_cert")] = config.Mqtt_RootCaCert;
    root[F("mqtt_lwt_topic")] = config.Mqtt_LwtTopic;
    root[F("mqtt_lwt_online")] = config.Mqtt_LwtValue_Online;
    root[F("mqtt_lwt_offline")] = config.Mqtt_LwtValue_Offline;
    root[F("mqtt_publish_interval")] = config.Mqtt_PublishInterval;
    root[F("mqtt_hass_enabled")] = config.Mqtt_Hass_Enabled;
    root[F("mqtt_hass_expire")] = config.Mqtt_Hass_Expire;
    root[F("mqtt_hass_retain")] = config.Mqtt_Hass_Retain;
    root[F("mqtt_hass_topic")] = config.Mqtt_Hass_Topic;
    root[F("mqtt_hass_individualpanels")] = config.Mqtt_Hass_IndividualPanels;

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
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        retMsg[F("code")] = WebApiError::GenericNoValueFound;
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > MQTT_JSON_DOC_SIZE) {
        retMsg[F("message")] = F("Data too large!");
        retMsg[F("code")] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(MQTT_JSON_DOC_SIZE);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        retMsg[F("code")] = WebApiError::GenericParseError;
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
            && root.containsKey("mqtt_lwt_topic")
            && root.containsKey("mqtt_lwt_online")
            && root.containsKey("mqtt_lwt_offline")
            && root.containsKey("mqtt_publish_interval")
            && root.containsKey("mqtt_hass_enabled")
            && root.containsKey("mqtt_hass_expire")
            && root.containsKey("mqtt_hass_retain")
            && root.containsKey("mqtt_hass_topic")
            && root.containsKey("mqtt_hass_individualpanels"))) {
        retMsg[F("message")] = F("Values are missing!");
        retMsg[F("code")] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("mqtt_enabled")].as<bool>()) {
        if (root[F("mqtt_hostname")].as<String>().length() == 0 || root[F("mqtt_hostname")].as<String>().length() > MQTT_MAX_HOSTNAME_STRLEN) {
            retMsg[F("message")] = F("MqTT Server must between 1 and " STR(MQTT_MAX_HOSTNAME_STRLEN) " characters long!");
            retMsg[F("code")] = WebApiError::MqttHostnameLength;
            retMsg[F("param")][F("max")] = MQTT_MAX_HOSTNAME_STRLEN;
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_username")].as<String>().length() > MQTT_MAX_USERNAME_STRLEN) {
            retMsg[F("message")] = F("Username must not longer then " STR(MQTT_MAX_USERNAME_STRLEN) " characters!");
            retMsg[F("code")] = WebApiError::MqttUsernameLength;
            retMsg[F("param")][F("max")] = MQTT_MAX_USERNAME_STRLEN;
            response->setLength();
            request->send(response);
            return;
        }
        if (root[F("mqtt_password")].as<String>().length() > MQTT_MAX_PASSWORD_STRLEN) {
            retMsg[F("message")] = F("Password must not longer then " STR(MQTT_MAX_PASSWORD_STRLEN) " characters!");
            retMsg[F("code")] = WebApiError::MqttPasswordLength;
            retMsg[F("param")][F("max")] = MQTT_MAX_PASSWORD_STRLEN;
            response->setLength();
            request->send(response);
            return;
        }
        if (root[F("mqtt_topic")].as<String>().length() > MQTT_MAX_TOPIC_STRLEN) {
            retMsg[F("message")] = F("Topic must not longer then " STR(MQTT_MAX_TOPIC_STRLEN) " characters!");
            retMsg[F("code")] = WebApiError::MqttTopicLength;
            retMsg[F("param")][F("max")] = MQTT_MAX_TOPIC_STRLEN;
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_topic")].as<String>().indexOf(' ') != -1) {
            retMsg[F("message")] = F("Topic must not contain space characters!");
            retMsg[F("code")] = WebApiError::MqttTopicCharacter;
            response->setLength();
            request->send(response);
            return;
        }

        if (!root[F("mqtt_topic")].as<String>().endsWith("/")) {
            retMsg[F("message")] = F("Topic must end with slash (/)!");
            retMsg[F("code")] = WebApiError::MqttTopicTrailingSlash;
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_port")].as<uint>() == 0 || root[F("mqtt_port")].as<uint>() > 65535) {
            retMsg[F("message")] = F("Port must be a number between 1 and 65535!");
            retMsg[F("code")] = WebApiError::MqttPort;
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_root_ca_cert")].as<String>().length() > MQTT_MAX_ROOT_CA_CERT_STRLEN) {
            retMsg[F("message")] = F("Certificate must not longer then " STR(MQTT_MAX_ROOT_CA_CERT_STRLEN) " characters!");
            retMsg[F("code")] = WebApiError::MqttCertificateLength;
            retMsg[F("param")][F("max")] = MQTT_MAX_ROOT_CA_CERT_STRLEN;
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_lwt_topic")].as<String>().length() > MQTT_MAX_TOPIC_STRLEN) {
            retMsg[F("message")] = F("LWT topic must not longer then " STR(MQTT_MAX_TOPIC_STRLEN) " characters!");
            retMsg[F("code")] = WebApiError::MqttLwtTopicLength;
            retMsg[F("param")][F("max")] = MQTT_MAX_TOPIC_STRLEN;
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_lwt_topic")].as<String>().indexOf(' ') != -1) {
            retMsg[F("message")] = F("LWT topic must not contain space characters!");
            retMsg[F("code")] = WebApiError::MqttLwtTopicCharacter;
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_lwt_online")].as<String>().length() > MQTT_MAX_LWTVALUE_STRLEN) {
            retMsg[F("message")] = F("LWT online value must not longer then " STR(MQTT_MAX_LWTVALUE_STRLEN) " characters!");
            retMsg[F("code")] = WebApiError::MqttLwtOnlineLength;
            retMsg[F("param")][F("max")] = MQTT_MAX_LWTVALUE_STRLEN;
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_lwt_offline")].as<String>().length() > MQTT_MAX_LWTVALUE_STRLEN) {
            retMsg[F("message")] = F("LWT offline value must not longer then " STR(MQTT_MAX_LWTVALUE_STRLEN) " characters!");
            retMsg[F("code")] = WebApiError::MqttLwtOfflineLength;
            retMsg[F("param")][F("max")] = MQTT_MAX_LWTVALUE_STRLEN;
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_publish_interval")].as<uint32_t>() < 5 || root[F("mqtt_publish_interval")].as<uint32_t>() > 65535) {
            retMsg[F("message")] = F("Publish interval must be a number between 5 and 65535!");
            retMsg[F("code")] = WebApiError::MqttPublishInterval;
            retMsg[F("param")][F("min")] = 5;
            retMsg[F("param")][F("max")] = 65535;
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_hass_enabled")].as<bool>()) {
            if (root[F("mqtt_hass_topic")].as<String>().length() > MQTT_MAX_TOPIC_STRLEN) {
                retMsg[F("message")] = F("Hass topic must not longer then " STR(MQTT_MAX_TOPIC_STRLEN) " characters!");
                retMsg[F("code")] = WebApiError::MqttHassTopicLength;
                retMsg[F("param")][F("max")] = MQTT_MAX_TOPIC_STRLEN;
                response->setLength();
                request->send(response);
                return;
            }

            if (root[F("mqtt_hass_topic")].as<String>().indexOf(' ') != -1) {
                retMsg[F("message")] = F("Hass topic must not contain space characters!");
                retMsg[F("code")] = WebApiError::MqttHassTopicCharacter;
                response->setLength();
                request->send(response);
                return;
            }
        }
    }

    CONFIG_T& config = Configuration.get();
    config.Mqtt_Enabled = root[F("mqtt_enabled")].as<bool>();
    config.Mqtt_Retain = root[F("mqtt_retain")].as<bool>();
    config.Mqtt_Tls = root[F("mqtt_tls")].as<bool>();
    strlcpy(config.Mqtt_RootCaCert, root[F("mqtt_root_ca_cert")].as<String>().c_str(), sizeof(config.Mqtt_RootCaCert));
    config.Mqtt_Port = root[F("mqtt_port")].as<uint>();
    strlcpy(config.Mqtt_Hostname, root[F("mqtt_hostname")].as<String>().c_str(), sizeof(config.Mqtt_Hostname));
    strlcpy(config.Mqtt_Username, root[F("mqtt_username")].as<String>().c_str(), sizeof(config.Mqtt_Username));
    strlcpy(config.Mqtt_Password, root[F("mqtt_password")].as<String>().c_str(), sizeof(config.Mqtt_Password));
    strlcpy(config.Mqtt_Topic, root[F("mqtt_topic")].as<String>().c_str(), sizeof(config.Mqtt_Topic));
    strlcpy(config.Mqtt_LwtTopic, root[F("mqtt_lwt_topic")].as<String>().c_str(), sizeof(config.Mqtt_LwtTopic));
    strlcpy(config.Mqtt_LwtValue_Online, root[F("mqtt_lwt_online")].as<String>().c_str(), sizeof(config.Mqtt_LwtValue_Online));
    strlcpy(config.Mqtt_LwtValue_Offline, root[F("mqtt_lwt_offline")].as<String>().c_str(), sizeof(config.Mqtt_LwtValue_Offline));
    config.Mqtt_PublishInterval = root[F("mqtt_publish_interval")].as<uint32_t>();
    config.Mqtt_Hass_Enabled = root[F("mqtt_hass_enabled")].as<bool>();
    config.Mqtt_Hass_Expire = root[F("mqtt_hass_expire")].as<bool>();
    config.Mqtt_Hass_Retain = root[F("mqtt_hass_retain")].as<bool>();
    config.Mqtt_Hass_IndividualPanels = root[F("mqtt_hass_individualpanels")].as<bool>();
    strlcpy(config.Mqtt_Hass_Topic, root[F("mqtt_hass_topic")].as<String>().c_str(), sizeof(config.Mqtt_Hass_Topic));
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");
    retMsg[F("code")] = WebApiError::GenericSuccess;

    response->setLength();
    request->send(response);

    MqttSettings.performReconnect();
    MqttHandleHass.forceUpdate();
}

String WebApiMqttClass::getRootCaCertInfo(const char* cert)
{
    char rootCaCertInfo[1024] = "";

    mbedtls_x509_crt global_cacert;

    strlcpy(rootCaCertInfo, "Can't parse root ca", sizeof(rootCaCertInfo));

    mbedtls_x509_crt_init(&global_cacert);
    int ret = mbedtls_x509_crt_parse(&global_cacert, const_cast<unsigned char*>((unsigned char*)cert), 1 + strlen(cert));
    if (ret < 0) {
        snprintf(rootCaCertInfo, sizeof(rootCaCertInfo), "Can't parse root ca: mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
        mbedtls_x509_crt_free(&global_cacert);
        return "";
    }
    mbedtls_x509_crt_info(rootCaCertInfo, sizeof(rootCaCertInfo) - 1, "", &global_cacert);
    mbedtls_x509_crt_free(&global_cacert);

    return rootCaCertInfo;
}
