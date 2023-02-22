// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "HoymilesRadio.h"
#include "inverters/InverterAbstract.h"
#include "types.h"
#include <Print.h>
#include <SPI.h>
#include <memory>
#include <vector>

#define HOY_SYSTEM_CONFIG_PARA_POLL_INTERVAL (2 * 60 * 1000) // 2 minutes
#define HOY_SYSTEM_CONFIG_PARA_POLL_MIN_DURATION (4 * 60 * 1000) // at least 4 minutes between sending limit command and read request. Otherwise eventlog entry

class HoymilesClass {
public:
    void init(SPIClass* initialisedSpiBus, uint8_t pinCE, uint8_t pinIRQ);
    void loop();

    void setMessageOutput(Print* output);
    Print* getMessageOutput();

    std::shared_ptr<InverterAbstract> addInverter(const char* name, uint64_t serial);
    std::shared_ptr<InverterAbstract> getInverterByPos(uint8_t pos);
    std::shared_ptr<InverterAbstract> getInverterBySerial(uint64_t serial);
    std::shared_ptr<InverterAbstract> getInverterByFragment(fragment_t* fragment);
    int8_t getInverterPosBySerial(uint64_t serial);
    void removeInverterBySerial(uint64_t serial);
    size_t getNumInverters();

    HoymilesRadio* getRadio();

    uint32_t PollInterval();
    void setPollInterval(uint32_t interval);

private:
    std::vector<std::shared_ptr<InverterAbstract>> _inverters;
    std::unique_ptr<HoymilesRadio> _radio;

    SemaphoreHandle_t _xSemaphore;

    uint32_t _pollInterval = 0;
    uint32_t _lastPoll = 0;

    Print* _messageOutput = &Serial;
};

extern HoymilesClass Hoymiles;