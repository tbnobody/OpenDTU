// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ArduinoJson.h>
#include <Hoymiles.h>
#include <TaskSchedulerDeclarations.h>

// mqtt discovery device classes
enum {
    DEVICE_CLS_NONE = 0,
    DEVICE_CLS_CURRENT,
    DEVICE_CLS_ENERGY,
    DEVICE_CLS_PWR,
    DEVICE_CLS_VOLTAGE,
    DEVICE_CLS_FREQ,
    DEVICE_CLS_TEMP,
    DEVICE_CLS_POWER_FACTOR,
    DEVICE_CLS_REACTIVE_POWER
};
const char* const deviceClasses[] = { 0, "current", "energy", "power", "voltage", "frequency", "temperature", "power_factor", "reactive_power" };
enum {
    STATE_CLS_NONE = 0,
    STATE_CLS_MEASUREMENT,
    STATE_CLS_TOTAL_INCREASING
};
const char* const stateClasses[] = { 0, "measurement", "total_increasing" };

typedef struct {
    FieldId_t fieldId; // field id
    uint8_t deviceClsId; // device class
    uint8_t stateClsId; // state class
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
    void publish(const String& subtopic, const String& payload);
    void publishDtuSensor(const char* name, const char* device_class, const char* category, const char* icon, const char* unit_of_measure, const char* subTopic);
    void publishDtuBinarySensor(const char* name, const char* device_class, const char* category, const char* payload_on, const char* payload_off, const char* subTopic = "");
    void publishInverterField(std::shared_ptr<InverterAbstract> inv, const ChannelType_t type, const ChannelNum_t channel, const byteAssign_fieldDeviceClass_t fieldType, const bool clear = false);
    void publishInverterButton(std::shared_ptr<InverterAbstract> inv, const char* caption, const char* icon, const char* category, const char* deviceClass, const char* subTopic, const char* payload);
    void publishInverterNumber(std::shared_ptr<InverterAbstract> inv, const char* caption, const char* icon, const char* category, const char* commandTopic, const char* stateTopic, const char* unitOfMeasure, const int16_t min = 1, const int16_t max = 100);
    void publishInverterBinarySensor(std::shared_ptr<InverterAbstract> inv, const char* caption, const char* subTopic, const char* payload_on, const char* payload_off);

    static void createInverterInfo(DynamicJsonDocument& doc, std::shared_ptr<InverterAbstract> inv);
    static void createDtuInfo(DynamicJsonDocument& doc);

    static void createDeviceInfo(DynamicJsonDocument& doc, const String& name, const String& identifiers, const String& configuration_url, const String& manufacturer, const String& model, const String& sw_version, const String& via_device = "");

    static String getDtuUniqueId();
    static String getDtuUrl();

    Task _loopTask;

    bool _wasConnected = false;
    bool _updateForced = false;
};

extern MqttHandleHassClass MqttHandleHass;
