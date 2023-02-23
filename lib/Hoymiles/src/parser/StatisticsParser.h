// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "Parser.h"
#include <Arduino.h>
#include <cstdint>
#include <list>

#define STATISTIC_PACKET_SIZE (4 * 16)

// units
enum UnitId_t {
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
enum FieldId_t {
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

const char* const field_metric_types[] = { "gauge", "gauge", "gauge", "counter", "counter",
    "gauge", "gauge", "gauge", "gauge", "gauge", "gauge", "gauge", "gauge", "gauge", "counter" };

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
enum ChannelNum_t {
    CH0 = 0,
    CH1,
    CH2,
    CH3,
    CH4
};

enum ChannelType_t {
    TYPE_AC = 0,
    TYPE_DC,
    TYPE_INV
};
const char* const channelsTypes[] = { "AC", "DC", "INV" };

typedef struct {
    ChannelType_t type;
    ChannelNum_t ch; // channel 0 - 4
    FieldId_t fieldId; // field id
    UnitId_t unitId; // uint id
    uint8_t start; // pos of first byte in buffer
    uint8_t num; // number of bytes in buffer
    uint16_t div; // divisor / calc command
    bool isSigned; // allow negative numbers
    uint8_t digits; // number of valid digits after the decimal point
} byteAssign_t;

typedef struct {
    ChannelType_t type;
    ChannelNum_t ch; // channel 0 - 4
    FieldId_t fieldId; // field id
    float offset; // offset (positive/negative) to be applied on the fetched value
} fieldSettings_t;

class StatisticsParser : public Parser {
public:
    void clearBuffer();
    void appendFragment(uint8_t offset, uint8_t* payload, uint8_t len);

    void setByteAssignment(const std::list<byteAssign_t>* byteAssignment);

    const byteAssign_t* getAssignmentByChannelField(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId);
    fieldSettings_t* getSettingByChannelField(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId);

    float getChannelFieldValue(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId);
    bool hasChannelFieldValue(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId);
    const char* getChannelFieldUnit(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId);
    const char* getChannelFieldName(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId);
    const char* getChannelFieldMetricType(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId);
    uint8_t getChannelFieldDigits(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId);

    float getChannelFieldOffset(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId);
    void setChannelFieldOffset(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId, float offset);

    std::list<ChannelType_t> getChannelTypes();
    const char* getChannelTypeName(ChannelType_t type);
    std::list<ChannelNum_t> getChannelsByType(ChannelType_t type);

    uint16_t getStringMaxPower(uint8_t channel);
    void setStringMaxPower(uint8_t channel, uint16_t power);

    void resetRxFailureCount();
    void incrementRxFailureCount();
    uint32_t getRxFailureCount();

private:
    uint8_t _payloadStatistic[STATISTIC_PACKET_SIZE] = {};
    uint8_t _statisticLength = 0;
    uint16_t _stringMaxPower[CH4];

    const std::list<byteAssign_t>* _byteAssignment;
    std::list<fieldSettings_t> _fieldSettings;

    uint32_t _rxFailureCount = 0;
};