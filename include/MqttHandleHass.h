// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ArduinoJson.h>
#include <Hoymiles.h>
#include <TaskSchedulerDeclarations.h>

// mqtt discovery device classes
enum DeviceClassType {
    DEVICE_CLS_NONE = 0,
    DEVICE_CLS_CURRENT,
    DEVICE_CLS_ENERGY,
    DEVICE_CLS_PWR,
    DEVICE_CLS_VOLTAGE,
    DEVICE_CLS_FREQ,
    DEVICE_CLS_TEMP,
    DEVICE_CLS_POWER_FACTOR,
    DEVICE_CLS_REACTIVE_POWER,
    DEVICE_CLS_CONNECTIVITY,
    DEVICE_CLS_DURATION,
    DEVICE_CLS_SIGNAL_STRENGTH,
    DEVICE_CLS_TEMPERATURE,
    DEVICE_CLS_RESTART
};
const char* const deviceClass_name[] = { 0, "current", "energy", "power", "voltage", "frequency", "temperature", "power_factor", "reactive_power", "connectivity", "duration", "signal_strength", "temperature", "restart" };

enum StateClassType {
    STATE_CLS_NONE = 0,
    STATE_CLS_MEASUREMENT,
    STATE_CLS_TOTAL_INCREASING
};
const char* const stateClass_name[] = { 0, "measurement", "total_increasing" };

enum CategoryType {
    CATEGORY_NONE = 0,
    CATEGORY_CONFIG,
    CATEGORY_DIAGNOSTIC
};
const char* const category_name[] = { 0, "config", "diagnostic" };


typedef struct {
    FieldId_t fieldId; // field id
    DeviceClassType deviceClsId; // device class
    StateClassType stateClsId; // state class
} byteAssign_fieldDeviceClass_t;

const byteAssign_fieldDeviceClass_t deviceFieldAssignment[] = {
    { FLD_UDC, DEVICE_CLS_VOLTAGE, STATE_CLS_MEASUREMENT },
    { FLD_IDC, DEVICE_CLS_CURRENT, STATE_CLS_MEASUREMENT },
    { FLD_PDC, DEVICE_CLS_PWR, STATE_CLS_MEASUREMENT },
    { FLD_YD, DEVICE_CLS_ENERGY, STATE_CLS_TOTAL_INCREASING },
    { FLD_YT, DEVICE_CLS_ENERGY, STATE_CLS_TOTAL_INCREASING },
    { FLD_UAC, DEVICE_CLS_VOLTAGE, STATE_CLS_MEASUREMENT },
    { FLD_IAC, DEVICE_CLS_CURRENT, STATE_CLS_MEASUREMENT },
    { FLD_PAC, DEVICE_CLS_PWR, STATE_CLS_MEASUREMENT },
    { FLD_F, DEVICE_CLS_FREQ, STATE_CLS_MEASUREMENT },
    { FLD_T, DEVICE_CLS_TEMP, STATE_CLS_MEASUREMENT },
    { FLD_PF, DEVICE_CLS_POWER_FACTOR, STATE_CLS_MEASUREMENT },
    { FLD_EFF, DEVICE_CLS_NONE, STATE_CLS_NONE },
    { FLD_IRR, DEVICE_CLS_NONE, STATE_CLS_NONE },
    { FLD_Q, DEVICE_CLS_REACTIVE_POWER, STATE_CLS_MEASUREMENT }
};
#define DEVICE_CLS_ASSIGN_LIST_LEN (sizeof(deviceFieldAssignment) / sizeof(byteAssign_fieldDeviceClass_t))

class MqttHandleHassClass {
public:
    MqttHandleHassClass();
    void init(Scheduler& scheduler);
    void publishConfig();
    void forceUpdate();

private:
    void loop();
    static void publish(const String& subtopic, const String& payload);
    static void publish(const String& subtopic, const JsonDocument& doc);

    static void addCommonMetadata(JsonDocument& doc, const String& unit_of_measure, const String& icon, const DeviceClassType device_class, const CategoryType category);

    // Binary Sensor
    static void publishBinarySensor(JsonDocument& doc, const String& root_device, const String& unique_id_prefix, const String& name, const String& state_topic, const String& payload_on, const String& payload_off, const DeviceClassType device_class, const CategoryType category);
    static void publishDtuBinarySensor(const String& name, const String& state_topic, const String& payload_on, const String& payload_off, const DeviceClassType device_class, const CategoryType category);
    static void publishInverterBinarySensor(std::shared_ptr<InverterAbstract> inv, const String& name, const String& state_topic, const String& payload_on, const String& payload_off, const DeviceClassType device_class, const CategoryType category);

    // Sensor
    static void publishSensor(JsonDocument& doc, const String& root_device, const String& unique_id_prefix, const String& name, const String& state_topic, const String& unit_of_measure, const String& icon, const DeviceClassType device_class, const CategoryType category);
    static void publishDtuSensor(const String& name, const String& state_topic, const String& unit_of_measure, const String& icon, const DeviceClassType device_class, const CategoryType category);
    static void publishInverterSensor(std::shared_ptr<InverterAbstract> inv, const String& name, const String& state_topic, const String& unit_of_measure, const String& icon, const DeviceClassType device_class, const CategoryType category);

    static void publishInverterField(std::shared_ptr<InverterAbstract> inv, const ChannelType_t type, const ChannelNum_t channel, const byteAssign_fieldDeviceClass_t fieldType, const bool clear = false);
    static void publishInverterButton(std::shared_ptr<InverterAbstract> inv, const String& name, const String& state_topic, const String& payload, const String& icon, const DeviceClassType device_class, const CategoryType category);
    static void publishInverterNumber(std::shared_ptr<InverterAbstract> inv, const String& name, const String& state_topic, const String& command_topic, const int16_t min, const int16_t max, float step, const String& unit_of_measure, const String& icon, const CategoryType category);

    static void createInverterInfo(JsonDocument& doc, std::shared_ptr<InverterAbstract> inv);
    static void createDtuInfo(JsonDocument& doc);

    static void createDeviceInfo(JsonDocument& doc, const String& name, const String& identifiers, const String& configuration_url, const String& manufacturer, const String& model, const String& sw_version, const String& via_device = "");

    static String getDtuUniqueId();
    static String getDtuUrl();

    Task _loopTask;

    bool _wasConnected = false;
    bool _updateForced = false;
};

extern MqttHandleHassClass MqttHandleHass;
