// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <espMqttClient.h>
#include <Arduino.h>
#include <map>
#include <list>
#include <mutex>
#include "SDM.h"
#include "sml.h"
#include <TaskSchedulerDeclarations.h>
#include <SoftwareSerial.h>

typedef struct {
  const unsigned char OBIS[6];
  void (*Fn)(double&);
  float* Arg;
} OBISHandler;

class PowerMeterClass {
public:
    enum class Source : unsigned {
        MQTT = 0,
        SDM1PH = 1,
        SDM3PH = 2,
        HTTP = 3,
        SML = 4,
        SMAHM2 = 5
    };
    void init(Scheduler& scheduler);
    float getPowerTotal(bool forceUpdate = true);
    uint32_t getLastPowerMeterUpdate();

private:
    void loop();
    void mqtt();

    void onMqttMessage(const espMqttClientTypes::MessageProperties& properties,
        const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total);

    Task _loopTask;

    bool _verboseLogging = true;
    uint32_t _lastPowerMeterCheck;
    // Used in Power limiter for safety check
    uint32_t _lastPowerMeterUpdate;

    float _powerMeter1Power = 0.0;
    float _powerMeter2Power = 0.0;
    float _powerMeter3Power = 0.0;
    float _powerMeter1Voltage = 0.0;
    float _powerMeter2Voltage = 0.0;
    float _powerMeter3Voltage = 0.0;
    float _powerMeterImport = 0.0;
    float _powerMeterExport = 0.0;

    std::map<String, float*> _mqttSubscriptions;

    mutable std::mutex _mutex;

    std::unique_ptr<SDM> _upSdm = nullptr;
    std::unique_ptr<SoftwareSerial> _upSmlSerial = nullptr;

    void readPowerMeter();

    bool smlReadLoop();
    const std::list<OBISHandler> smlHandlerList{
        {{0x01, 0x00, 0x10, 0x07, 0x00, 0xff}, &smlOBISW, &_powerMeter1Power},
        {{0x01, 0x00, 0x01, 0x08, 0x00, 0xff}, &smlOBISWh, &_powerMeterImport},
        {{0x01, 0x00, 0x02, 0x08, 0x00, 0xff}, &smlOBISWh, &_powerMeterExport}
    };
};

extern PowerMeterClass PowerMeter;
