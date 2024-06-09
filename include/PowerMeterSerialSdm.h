// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <SoftwareSerial.h>
#include "Configuration.h"
#include "PowerMeterProvider.h"
#include "SDM.h"

class PowerMeterSerialSdm : public PowerMeterProvider {
public:
    enum class Phases {
        One,
        Three
    };

    PowerMeterSerialSdm(Phases phases, PowerMeterSerialSdmConfig const& cfg)
        : _phases(phases)
        , _cfg(cfg) { }

    ~PowerMeterSerialSdm();

    bool init() final;
    void loop() final { } // polling is performed asynchronously
    float getPowerTotal() const final;
    bool isDataValid() const final;
    void doMqttPublish() const final;

private:
    static void pollingLoopHelper(void* context);
    bool readValue(std::unique_lock<std::mutex>& lock, uint16_t reg, float& targetVar);
    std::atomic<bool> _taskDone;
    void pollingLoop();

    Phases _phases;
    PowerMeterSerialSdmConfig const _cfg;

    uint32_t _lastPoll = 0;

    float _phase1Power = 0.0;
    float _phase2Power = 0.0;
    float _phase3Power = 0.0;
    float _phase1Voltage = 0.0;
    float _phase2Voltage = 0.0;
    float _phase3Voltage = 0.0;
    float _energyImport = 0.0;
    float _energyExport = 0.0;

    mutable std::mutex _valueMutex;

    std::unique_ptr<SoftwareSerial> _upSdmSerial = nullptr;
    std::unique_ptr<SDM> _upSdm = nullptr;

    TaskHandle_t _taskHandle = nullptr;
    bool _stopPolling;
    mutable std::mutex _pollingMutex;
    std::condition_variable _cv;
};
