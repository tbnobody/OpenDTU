#pragma once

#include <cstdint>
#include "Configuration.h"
#include "ModbusServerTCPasync.h"
#include "ModbusMessage.h"
#include "Hoymiles.h"

struct SunspecConstants {
    static const uint8_t BASE_UNIT_ID = 126; // default: 126
    static const uint16_t COMMON_BASE_ADDRESS = 40000;
    static const uint16_t COMMON_MODEL_BASE_ADDRESS = COMMON_BASE_ADDRESS + 2;
    static const uint16_t INVERTER_MODEL_10X_BASE_ADDRESS = COMMON_BASE_ADDRESS + 70;
    static const uint16_t INVERTER_MODEL_11X_BASE_ADDRESS = COMMON_BASE_ADDRESS + 110;
    static const uint16_t INVERTER_MODEL_IMMEDIATE_CONTROLS_BASE_ADDRESS = COMMON_BASE_ADDRESS + 184;

};

class SunspecModel {
private:
    std::map<uint16_t, uint16_t>& _registers;
    uint8_t _unitId;
    uint16_t _startAddr;
    std::vector<uint16_t> _buffer;

public:
    SunspecModel(std::map<uint16_t, uint16_t>& registers, uint8_t unitId, uint16_t modelId, uint16_t startAddr);
    ~SunspecModel();
    SunspecModel& add(uint16_t value);
    SunspecModel& addUInt32(uint32_t value);
    SunspecModel& add(String utf8value, unsigned int maxRegs);
    SunspecModel& addFloat(float value);
    uint16_t commit();
};

class SunspecApiClass {
private:
    const unsigned long UPDATE_INTERVAL_MILLIS = 5000L;

    // SunSpec scale factors
    const int16_t acCurrentScaleFactor = -2;
    const int16_t acVoltageScaleFactor = -1;
    const int16_t acPowerScaleFactor = -1;
    const int16_t acFrequencyScaleFactor = -2;
    const int16_t acReactivePowerScaleFactor = -1;
    const int16_t acPowerLimitScaleFactor = -2;
    const int16_t acPowerFactorScaleFactor = -2;

    uint16_t _powerLimitPct = 100;
    unsigned long _lastPublish = 0;
    unsigned long _lastLoopMillis = 0;
    std::map<uint8_t, std::map<uint16_t, uint16_t>> _memory;

public:
    void init();
    void loop();
private:
    ModbusMessage readHoldRegister(ModbusMessage request);
    ModbusMessage writeHoldRegister(ModbusMessage request);
    ModbusMessage writeMultipleRegisters(ModbusMessage request);

    void sunspecHeader(uint8_t unitId, uint16_t startAddr);
    SunspecModel newModel(uint8_t unitId, uint16_t modelId, uint16_t startAddr);

    float inverterFieldValue(std::shared_ptr<InverterAbstract> inv, ChannelType_t type, ChannelNum_t channel, FieldId_t field, float factor = 1.0);
    uint8_t determinePhasesCount(std::shared_ptr<InverterAbstract> inv);
    float calcDcPowerChannelSum(std::shared_ptr<InverterAbstract> inv);
    float calcDcVoltageChannelAverage(std::shared_ptr<InverterAbstract> inv);
    void updateRegisters(uint8_t unitId);
    void updateAllRegisters();
    void updateReadResponse(ModbusMessage& response, uint8_t unitId, uint16_t addr, uint16_t words);
    ModbusMessage error(String message, uint8_t unitId, ModbusMessage request, Error errorCode, ModbusMessage response);
    void writeRegister(uint8_t unitId, uint16_t addr, uint16_t value);
};

extern SunspecApiClass SunspecApi;
