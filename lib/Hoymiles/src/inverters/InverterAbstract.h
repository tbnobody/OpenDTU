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
    FLD_YW,
    FLD_YT,
    FLD_UAC,
    FLD_IAC,
    FLD_PAC,
    FLD_F,
    FLD_T,
    FLD_PCT,
    FLD_EFF,
    FLD_IRR };
const char* const fields[] = { "U_DC", "I_DC", "P_DC", "YieldDay", "YieldWeek", "YieldTotal",
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

class InverterAbstract {
public:
    void setSerial(uint64_t serial);
    uint64_t serial();
    void setName(const char* name);
    const char* name();
    virtual String typeName() = 0;
    virtual const byteAssign_t* getByteAssignment() = 0;
    void clearRxFragmentBuffer();

private:
    serial_u _serial;
    char _name[MAX_NAME_LENGTH];
};