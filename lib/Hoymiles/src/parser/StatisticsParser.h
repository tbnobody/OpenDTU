// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "Parser.h"
#include <Arduino.h>
#include <cstdint>

#define STATISTIC_PACKET_SIZE (4 * 16)

// units
enum {
    UNIT_V = 0,
    UNIT_A,
    UNIT_W,
    UNIT_WH,
    UNIT_KWH,
    UNIT_HZ,
    UNIT_C,
    UNIT_PCT,
    UNIT_VA,
    UNIT_NONE
};
const char* const units[] = { "V", "A", "W", "Wh", "kWh", "Hz", "°C", "%", "var", "" };

// field types
enum {
    FLD_UDC = 0,
    FLD_IDC,
    FLD_PDC,
    FLD_YD,
    FLD_YT,
    FLD_UAC,
    FLD_IAC,
    FLD_PAC,
    FLD_F,
    FLD_T,
    FLD_PF,
    FLD_EFF,
    FLD_IRR,
    FLD_PRA,
    FLD_EVT_LOG
};
const char* const fields[] = { "Voltage", "Current", "Power", "YieldDay", "YieldTotal",
    "Voltage", "Current", "Power", "Frequency", "Temperature", "PowerFactor", "Efficiency", "Irradiation", "ReactivePower", "EventLogCount" };

// indices to calculation functions, defined in hmInverter.h
enum {
    CALC_YT_CH0 = 0,
    CALC_YD_CH0,
    CALC_UDC_CH,
    CALC_PDC_CH0,
    CALC_EFF_CH0,
    CALC_IRR_CH
};
enum { CMD_CALC = 0xffff };

// CH0 is default channel (freq, ac, temp)
enum {
    CH0 = 0,
    CH1,
    CH2,
    CH3,
    CH4
};

typedef struct {
    uint8_t fieldId; // field id
    uint8_t unitId; // uint id
    uint8_t ch; // channel 0 - 4
    uint8_t start; // pos of first byte in buffer
    uint8_t num; // number of bytes in buffer
    uint16_t div; // divisor / calc command
    bool isSigned; // allow negative numbers
} byteAssign_t;

class StatisticsParser : public Parser {
public:
    void clearBuffer();
    void appendFragment(uint8_t offset, uint8_t* payload, uint8_t len);

    void setByteAssignment(const byteAssign_t* byteAssignment, const uint8_t count);

    uint8_t getAssignIdxByChannelField(uint8_t channel, uint8_t fieldId);
    float getChannelFieldValue(uint8_t channel, uint8_t fieldId);
    bool hasChannelFieldValue(uint8_t channel, uint8_t fieldId);
    const char* getChannelFieldUnit(uint8_t channel, uint8_t fieldId);
    const char* getChannelFieldName(uint8_t channel, uint8_t fieldId);
    uint8_t getChannelFieldDigits(uint8_t channel, uint8_t fieldId);

    uint8_t getChannelCount();

    uint16_t getChannelMaxPower(uint8_t channel);
    void setChannelMaxPower(uint8_t channel, uint16_t power);

    void resetRxFailureCount();
    void incrementRxFailureCount();
    uint32_t getRxFailureCount();

private:
    uint8_t _payloadStatistic[STATISTIC_PACKET_SIZE] = {};
    uint8_t _statisticLength = 0;
    uint16_t _chanMaxPower[CH4];

    const byteAssign_t* _byteAssignment;
    uint8_t _byteAssignmentCount;

    uint32_t _rxFailureCount = 0;
};