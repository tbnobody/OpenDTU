// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "Parser.h"
#include <cstdint>
#include <list>

#define STATISTIC_PACKET_SIZE (7 * 16)

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
    UNIT_VAR,
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
    FLD_Q,
    FLD_EVT_LOG,
    // HMT only
    FLD_UAC_1N,
    FLD_UAC_2N,
    FLD_UAC_3N,
    FLD_UAC_12,
    FLD_UAC_23,
    FLD_UAC_31,
    FLD_IAC_1,
    FLD_IAC_2,
    FLD_IAC_3
};
const char* const fields[] = { "Voltage", "Current", "Power", "YieldDay", "YieldTotal",
    "Voltage", "Current", "Power", "Frequency", "Temperature", "PowerFactor", "Efficiency", "Irradiation", "ReactivePower", "EventLogCount",
    "Voltage Ph1-N", "Voltage Ph2-N", "Voltage Ph3-N", "Voltage Ph1-Ph2", "Voltage Ph2-Ph3", "Voltage Ph3-Ph1", "Current Ph1", "Current Ph2", "Current Ph3" };

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
    CH4,
    CH5,
    CH_CNT
};

enum ChannelType_t {
    TYPE_AC = 0,
    TYPE_DC,
    TYPE_INV
};
const char* const channelsTypes[] = { "AC", "DC", "INV" };

typedef struct {
    ChannelType_t type;
    ChannelNum_t ch; // channel 0 - 5
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
    ChannelNum_t ch; // channel 0 - 5
    FieldId_t fieldId; // field id
    float offset; // offset (positive/negative) to be applied on the fetched value
} fieldSettings_t;

class StatisticsParser : public Parser {
public:
    StatisticsParser();
    void clearBuffer();
    void appendFragment(uint8_t offset, uint8_t* payload, uint8_t len);

    void setByteAssignment(const byteAssign_t* byteAssignment, uint8_t size);

    // Returns 1 based amount of expected bytes of statistic data
    uint8_t getExpectedByteCount();

    const byteAssign_t* getAssignmentByChannelField(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId);
    fieldSettings_t* getSettingByChannelField(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId);

    float getChannelFieldValue(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId);
    String getChannelFieldValueString(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId);
    bool hasChannelFieldValue(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId);
    const char* getChannelFieldUnit(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId);
    const char* getChannelFieldName(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId);
    uint8_t getChannelFieldDigits(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId);

    bool setChannelFieldValue(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId, float value);

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

    void zeroRuntimeData();
    void zeroDailyData();

    // Update time when new data from the inverter is received
    void setLastUpdate(uint32_t lastUpdate);

    // Update time when internal data structure changes (from inverter and by internal manipulation)
    uint32_t getLastUpdateFromInternal();
    void setLastUpdateFromInternal(uint32_t lastUpdate);

private:
    void zeroFields(const FieldId_t* fields);

    uint8_t _payloadStatistic[STATISTIC_PACKET_SIZE] = {};
    uint8_t _statisticLength = 0;
    uint16_t _stringMaxPower[CH_CNT];

    const byteAssign_t* _byteAssignment;
    uint8_t _byteAssignmentSize;
    uint8_t _expectedByteCount = 0;
    std::list<fieldSettings_t> _fieldSettings;

    uint32_t _rxFailureCount = 0;
    uint32_t _lastUpdateFromInternal = 0;
};