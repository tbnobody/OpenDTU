#pragma once

#include "types.h"
#include <Arduino.h>
#include <cstdint>

#define MAX_NAME_LENGTH 32

// units
enum { UNIT_V = 0,
    UNIT_A,
    UNIT_W,
    UNIT_WH,
    UNIT_KWH,
    UNIT_HZ,
    UNIT_C,
    UNIT_PCT };
const char* const units[] = { "V", "A", "W", "Wh", "kWh", "Hz", "°C", "%" };

// field types
enum { FLD_UDC = 0,
    FLD_IDC,
    FLD_PDC,
    FLD_YD,
    FLD_YT,
    FLD_UAC,
    FLD_IAC,
    FLD_PAC,
    FLD_F,
    FLD_T,
    FLD_PCT,
    FLD_EFF,
    FLD_IRR };
const char* const fields[] = { "U_DC", "I_DC", "P_DC", "YieldDay", "YieldTotal",
    "U_AC", "I_AC", "P_AC", "Freq", "Temp", "Pct", "Effiency", "Irradiation" };

// indices to calculation functions, defined in hmInverter.h
enum { CALC_YT_CH0 = 0,
    CALC_YD_CH0,
    CALC_UDC_CH,
    CALC_PDC_CH0,
    CALC_EFF_CH0,
    CALC_IRR_CH };
enum { CMD_CALC = 0xffff };

// CH0 is default channel (freq, ac, temp)
enum { CH0 = 0,
    CH1,
    CH2,
    CH3,
    CH4 };

typedef struct {
    uint8_t fieldId; // field id
    uint8_t unitId; // uint id
    uint8_t ch; // channel 0 - 4
    uint8_t start; // pos of first byte in buffer
    uint8_t num; // number of bytes in buffer
    uint16_t div; // divisor / calc command
} byteAssign_t;

#define MAX_RF_FRAGMENT_COUNT 5
#define MAX_RETRANSMIT_COUNT 5

class InverterAbstract;

// prototypes
static float calcYieldTotalCh0(InverterAbstract* iv, uint8_t arg0);
static float calcYieldDayCh0(InverterAbstract* iv, uint8_t arg0);
static float calcUdcCh(InverterAbstract* iv, uint8_t arg0);
static float calcPowerDcCh0(InverterAbstract* iv, uint8_t arg0);
static float calcEffiencyCh0(InverterAbstract* iv, uint8_t arg0);
static float calcIrradiation(InverterAbstract* iv, uint8_t arg0);

using func_t = float(InverterAbstract*, uint8_t);

struct calcFunc_t {
    uint8_t funcId; // unique id
    func_t* func; // function pointer
};

const calcFunc_t calcFunctions[] = {
    { CALC_YT_CH0, &calcYieldTotalCh0 },
    { CALC_YD_CH0, &calcYieldDayCh0 },
    { CALC_UDC_CH, &calcUdcCh },
    { CALC_PDC_CH0, &calcPowerDcCh0 },
    { CALC_EFF_CH0, &calcEffiencyCh0 },
    { CALC_IRR_CH, &calcIrradiation }
};

class InverterAbstract {
public:
    InverterAbstract(uint64_t serial);
    uint64_t serial();
    void setName(const char* name);
    const char* name();
    virtual String typeName() = 0;
    virtual const byteAssign_t* getByteAssignment() = 0;
    virtual const uint8_t getAssignmentCount() = 0;
    uint8_t getChannelCount();
    uint16_t getChannelMaxPower(uint8_t channel);
    void setChannelMaxPower(uint8_t channel, uint16_t power);

    void clearRxFragmentBuffer();
    void addRxFragment(uint8_t fragment[], uint8_t len);
    uint8_t verifyAllFragments();

    uint8_t getAssignIdxByChannelField(uint8_t channel, uint8_t fieldId);
    float getValue(uint8_t channel, uint8_t fieldId);
    bool hasValue(uint8_t channel, uint8_t fieldId);
    const char* getUnit(uint8_t channel, uint8_t fieldId);
    const char* getName(uint8_t channel, uint8_t fieldId);

    uint32_t getLastStatsUpdate();

private:
    serial_u _serial;
    char _name[MAX_NAME_LENGTH];
    fragment_t _rxFragmentBuffer[MAX_RF_FRAGMENT_COUNT];
    uint8_t _rxFragmentMaxPacketId = 0;
    uint8_t _rxFragmentLastPacketId = 0;
    uint8_t _rxFragmentRetransmitCnt = 0;

    uint8_t _payloadStats[MAX_RF_FRAGMENT_COUNT * MAX_RF_PAYLOAD_SIZE];
    uint32_t _lastStatsUpdate = 0;
    uint16_t _chanMaxPower[CH4];
};