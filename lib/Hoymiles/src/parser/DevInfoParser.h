// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "Parser.h"

#define DEV_INFO_SIZE 20

class DevInfoParser : public Parser {
public:
    DevInfoParser();
    void clearBufferAll();
    void appendFragmentAll(const uint8_t offset, const uint8_t* payload, const uint8_t len);

    void clearBufferSimple();
    void appendFragmentSimple(const uint8_t offset, const uint8_t* payload, const uint8_t len);

    uint32_t getLastUpdateAll() const;
    void setLastUpdateAll(const uint32_t lastUpdate);

    uint32_t getLastUpdateSimple() const;
    void setLastUpdateSimple(const uint32_t lastUpdate);

    uint16_t getFwBuildVersion() const;
    time_t getFwBuildDateTime() const;
    String getFwBuildDateTimeStr() const;
    uint16_t getFwBootloaderVersion() const;

    uint32_t getHwPartNumber() const;
    String getHwVersion() const;

    uint16_t getMaxPower() const;
    String getHwModelName() const;

    bool containsValidData() const;

private:
    static time_t timegm(const struct tm* tm);
    uint8_t getDevIdx() const;

    uint32_t _lastUpdateAll = 0;
    uint32_t _lastUpdateSimple = 0;

    uint8_t _payloadDevInfoAll[DEV_INFO_SIZE] = {};
    uint8_t _devInfoAllLength = 0;

    uint8_t _payloadDevInfoSimple[DEV_INFO_SIZE] = {};
    uint8_t _devInfoSimpleLength = 0;
};
