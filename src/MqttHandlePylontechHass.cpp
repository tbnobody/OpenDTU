// SPDX-License-Identifier: GPL-2.0-or-later

#include "PylontechCanReceiver.h"
#include "Battery.h"
#include "MqttHandlePylontechHass.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include "MessageOutput.h"

MqttHandlePylontechHassClass MqttHandlePylontechHass;

void MqttHandlePylontechHassClass::init()
{
}

void MqttHandlePylontechHassClass::loop()
{
    CONFIG_T& config = Configuration.get();
    if (!config.Battery_Enabled) {
        return;
    }
    if (_updateForced) {
        publishConfig();
        _updateForced = false;
    }

    if (MqttSettings.getConnected() && !_wasConnected) {
        // Connection established
        _wasConnected = true;
        publishConfig();
    } else if (!MqttSettings.getConnected() && _wasConnected) {
        // Connection lost
        _wasConnected = false;
    }
}

void MqttHandlePylontechHassClass::forceUpdate()
{
    _updateForced = true;
}

void MqttHandlePylontechHassClass::publishConfig()
{
    CONFIG_T& config = Configuration.get();
    if ((!config.Mqtt_Hass_Enabled) || (!config.Battery_Enabled)) {
        return;
    }

    if (!MqttSettings.getConnected()) {
        return;
    }

    // device info
    publishSensor("Manufacturer", "manufacturer");

    // battery info
    publishSensor("Battery voltage", "voltage", "voltage", "measurement", "V");
    publishSensor("Battery current", "current", "current", "measurement", "A");
    publishSensor("Temperature", "temperature", "temperature", "measurement", "°C");
    publishSensor("State of Charge (SOC)", "stateOfCharge", "battery", "measurement", "%");
    publishSensor("State of Health (SOH)", "stateOfHealth", NULL, "measurement", "%");
    publishSensor("Charge voltage (BMS)", "settings/chargeVoltage", "voltage", "measurement", "V");
    publishSensor("Charge current limit", "settings/chargeCurrentLimitation", "current", "measurement", "A");
    publishSensor("Discharge current limit", "settings/dischargeCurrentLimitation", "current", "measurement", "A");

    publishBinarySensor("Alarm Discharge current", "alarm/overCurrentDischarge", "1", "0");
    publishBinarySensor("Warning Discharge current", "warning/highCurrentDischarge", "1", "0");

    publishBinarySensor("Alarm Temperature low", "alarm/underTemperature", "1", "0");
    publishBinarySensor("Warning Temperature low", "warning/lowTemperature", "1", "0");

    publishBinarySensor("Alarm Temperature high", "alarm/overTemperature", "1", "0");
    publishBinarySensor("Warning Temperature high", "warning/highTemperature", "1", "0");

    publishBinarySensor("Alarm Voltage low", "alarm/underVoltage", "1", "0");
    publishBinarySensor("Warning Voltage low", "warning/lowVoltage", "1", "0");

    publishBinarySensor("Alarm Voltage high", "alarm/overVoltage", "1", "0");
    publishBinarySensor("Warning Voltage high", "warning/highVoltage", "1", "0");

    publishBinarySensor("Alarm BMS internal", "alarm/bmsInternal", "1", "0");
    publishBinarySensor("Warning BMS internal", "warning/bmsInternal", "1", "0");

    publishBinarySensor("Alarm High charge current", "alarm/overCurrentCharge", "1", "0");
    publishBinarySensor("Warning High charge current", "warning/highCurrentCharge", "1", "0");

    publishBinarySensor("Charge enabled", "charging/chargeEnabled", "1", "0");
    publishBinarySensor("Discharge enabled", "charging/dischargeEnabled", "1", "0");
    publishBinarySensor("Charge immediately", "charging/chargeImmediately", "1", "0");

    yield();
}

void MqttHandlePylontechHassClass::publishSensor(const char* caption, const char* subTopic, const char* deviceClass, const char* stateClass, const char* unitOfMeasurement )
{
    String sensorId = caption;
    sensorId.replace(" ", "_");
    sensorId.replace(".", "");
    sensorId.replace("(", "");
    sensorId.replace(")", "");
    sensorId.toLowerCase();

    String configTopic = "sensor/dtu_battery_" + serial
        + "/" + sensorId
        + "/config";

    String statTopic = MqttSettings.getPrefix() + "battery/";
    // omit serial to avoid a breaking change
    // statTopic.concat(serial);
    // statTopic.concat("/");
    statTopic.concat(subTopic);

    DynamicJsonDocument root(1024);
    root[F("name")] = caption;
    root[F("stat_t")] = statTopic;
    root[F("uniq_id")] = serial + "_" + sensorId;

    if (unitOfMeasurement != NULL) {
        root[F("unit_of_meas")] = unitOfMeasurement;
    }

    JsonObject deviceObj = root.createNestedObject("dev");
    createDeviceInfo(deviceObj);

    if (Configuration.get().Mqtt_Hass_Expire) {
        root[F("exp_aft")] = Configuration.get().Mqtt_PublishInterval * 3;
    }
    if (deviceClass != NULL) {
        root[F("dev_cla")] = deviceClass;
    }
    if (stateClass != NULL) {
        root[F("stat_cla")] = stateClass;
    }

    char buffer[512];
    serializeJson(root, buffer);
    publish(configTopic, buffer);

}
void MqttHandlePylontechHassClass::publishBinarySensor(const char* caption, const char* subTopic, const char* payload_on, const char* payload_off)
{
    String sensorId = caption;
    sensorId.replace(" ", "_");
    sensorId.replace(".", "");
    sensorId.replace("(", "");
    sensorId.replace(")", "");
    sensorId.toLowerCase();

    String configTopic = "binary_sensor/dtu_battery_" + serial
        + "/" + sensorId
        + "/config";

    String statTopic = MqttSettings.getPrefix() + "battery/";
    // omit serial to avoid a breaking change
    // statTopic.concat(serial);
    // statTopic.concat("/");
    statTopic.concat(subTopic);

    DynamicJsonDocument root(1024);
    root[F("name")] = caption;
    root[F("uniq_id")] = serial + "_" + sensorId;
    root[F("stat_t")] = statTopic;
    root[F("pl_on")] = payload_on;
    root[F("pl_off")] = payload_off;

    JsonObject deviceObj = root.createNestedObject("dev");
    createDeviceInfo(deviceObj);

    char buffer[512];
    serializeJson(root, buffer);
    publish(configTopic, buffer);
}

void MqttHandlePylontechHassClass::createDeviceInfo(JsonObject& object)
{
    object[F("name")] = "Battery(" + serial + ")";
    object[F("ids")] = serial;
    object[F("cu")] = String(F("http://")) + NetworkSettings.localIP().toString();
    object[F("mf")] = F("OpenDTU");
    object[F("mdl")] = Battery.getStats()->getManufacturer();
    object[F("sw")] = AUTO_GIT_HASH;
}

void MqttHandlePylontechHassClass::publish(const String& subtopic, const String& payload)
{
    String topic = Configuration.get().Mqtt_Hass_Topic;
    topic += subtopic;
    MqttSettings.publishGeneric(topic.c_str(), payload.c_str(), Configuration.get().Mqtt_Hass_Retain);
}
