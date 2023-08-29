// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "Parser.h"
#include <Arduino.h>

#define DEV_INFO_SIZE 20

class DevInfoParser : public Parser {
public:
    DevInfoParser();
    void clearBufferAll();
    void appendFragmentAll(uint8_t offset, uint8_t* payload, uint8_t len);

    void clearBufferSimple();
    void appendFragmentSimple(uint8_t offset, uint8_t* payload, uint8_t len);

    void beginAppendFragment();
    void endAppendFragment();

    uint32_t getLastUpdateAll();
    void setLastUpdateAll(uint32_t lastUpdate);

    uint32_t getLastUpdateSimple();
    void setLastUpdateSimple(uint32_t lastUpdate);

    uint16_t getFwBuildVersion();
    time_t getFwBuildDateTime();
    uint16_t getFwBootloaderVersion();

    uint32_t getHwPartNumber();
    String getHwVersion();

    uint16_t getMaxPower();
    String getHwModelName();

    bool containsValidData();

private:
    time_t timegm(struct tm* tm);
    uint8_t getDevIdx();

    uint32_t _lastUpdateAll = 0;
    uint32_t _lastUpdateSimple = 0;

    uint8_t _payloadDevInfoAll[DEV_INFO_SIZE] = {};
    uint8_t _devInfoAllLength = 0;

    uint8_t _payloadDevInfoSimple[DEV_INFO_SIZE] = {};
    uint8_t _devInfoSimpleLength = 0;

    SemaphoreHandle_t _xSemaphore;
};