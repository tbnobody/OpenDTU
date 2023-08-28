#pragma once

#include <cstdint>
#include "Configuration.h"
#include "ModbusServerTCPasync.h"
#include "ModbusMessage.h"
#include "Hoymiles.h"
#include "MessageOutput.h"

struct SunspecConstants {
    static const uint8_t BASE_UNIT_ID = 126; // default: 126
    static const uint16_t COMMON_BASE_ADDRESS = 40000;
    static const uint16_t INVERTER_MODEL_IMMEDIATE_CONTROLS_BASE_ADDRESS = COMMON_BASE_ADDRESS + 184;
};

namespace Modbus
{

class Value {
public:
    virtual ~Value() {}
    virtual uint16_t getValue(uint16_t offset) const = 0;
    virtual uint16_t regCount() const;
    virtual void update() {};
};

class UInt16Value : public Value {
private:
    const uint16_t _value;

public:
    UInt16Value(uint16_t value): _value(value) {}
    uint16_t getValue(uint16_t offset) const override {
        return _value;
    }
    uint16_t regCount() const override {
        return 1;
    }
};

class Int16Value : public Value {
private:
    const int16_t _value;

public:
    Int16Value(int16_t value): _value(value) {}
    uint16_t getValue(uint16_t offset) const override {
        return (uint16_t)_value;
    }
    uint16_t regCount() const override {
        return 1;
    }
};

class UInt32Value : public Value {
private:
    const uint32_t _value;

public:
    UInt32Value(uint32_t value): _value(value) {}

    uint16_t getValue(uint16_t offset) const override;

    uint16_t regCount() const override {
        return 2;
    }
};

class Float32Value : public Value {
private:
    const float _value;

public:
    Float32Value(float value): _value(value) {}

    uint16_t getValue(uint16_t offset) const override;
    
    uint16_t regCount() const override {
        return 2;
    }
};

class StringValue : public Value {
private:
    const String _value;
    const uint16_t _regCount;

public:
    StringValue(String utf8value, uint16_t regCount): _value(utf8value), _regCount(regCount) {}

    uint16_t getValue(uint16_t offset) const override;
    
    uint16_t regCount() const override {
        return _regCount;
    }
};

class UInt16Callback : public Value {
private:
    std::function<uint16_t()> _callback;

public:
    UInt16Callback(std::function<uint16_t()> callback) : _callback(callback) {}

    uint16_t getValue(uint16_t offset) const override {
        return _callback();
    }

    uint16_t regCount() const override {
        return 1;
    }
};

class FloatCallback : public Value {
private:
    std::function<float()> _callback;
    float _currentValue = NAN;

public:
    FloatCallback(std::function<float()> callback) : _callback(callback) {}

    void update() {
        _currentValue = _callback();
    }

    uint16_t getValue(uint16_t offset) const override;

    uint16_t regCount() const override {
        return 2;
    }
};

class CompositeValue : public Value {
private:
    std::vector<std::shared_ptr<Value>> _values;

public:
    CompositeValue& addValue(std::shared_ptr<Value> value) {
        _values.push_back(std::move(value));
        return *this;
    }

    CompositeValue& addString(String value, uint16_t regCount) {
        return addValue(std::make_shared<StringValue>(value, regCount));
    }

    CompositeValue& addUInt16(uint16_t value) {
        return addValue(std::make_shared<UInt16Value>(value));
    }

    CompositeValue& addUInt16(std::function<uint16_t()> callback) {
        return addValue(std::make_shared<UInt16Callback>(callback));
    }

    CompositeValue& addInt16(int16_t value) {
        return addValue(std::make_shared<Int16Value>(value));
    }

    CompositeValue& addUInt32(uint32_t value) {
        return addValue(std::make_shared<UInt32Value>(value));
    }

    CompositeValue& addFloat(float value) {
        return addValue(std::make_shared<Float32Value>(value));
    }

    CompositeValue& addFloat(std::function<float()> callback) {
        return addValue(std::make_shared<FloatCallback>(callback));
    }

    uint16_t getValue(uint16_t offset) const override;

    uint16_t regCount() const override;
};

class DeviceModel {
private:
    std::shared_ptr<CompositeValue> _delegate = std::make_shared<CompositeValue>();
public:
    DeviceModel() {
        _delegate->addString("SunS", 2);
    }

    DeviceModel& addSunspecModel(uint16_t modelId, std::shared_ptr<CompositeValue> model) {
        _delegate->addUInt16(modelId);
        _delegate->addUInt16(model->regCount());
        _delegate->addValue(model);
        return *this;
    }

    std::shared_ptr<CompositeValue> build() {
        _delegate->addUInt16(0xFFFF); // end marker
        _delegate->addUInt16(0);
        return _delegate;
     }
};

} // namespace Modbus

class SunspecApiClass {
private:
    const uint32_t UPDATE_INTERVAL_MILLIS = 5000L;

    // SunSpec scale factors
    const int16_t acCurrentScaleFactor = -2;
    const int16_t acVoltageScaleFactor = -1;
    const int16_t acPowerScaleFactor = -1;
    const int16_t acFrequencyScaleFactor = -2;
    const int16_t acReactivePowerScaleFactor = -1;
    const int16_t acLifetimeEnergyScaleFactor = 2;
    const int16_t acPowerLimitScaleFactor = -2;
    const int16_t acPowerFactorScaleFactor = -2;

    uint32_t _lastPublish = 0;
    uint32_t _lastLoopMillis = 0;
    uint32_t _reg = 0;

    uint16_t _powerLimitPct = 100;
    std::map<uint8_t, std::shared_ptr<Modbus::CompositeValue>> _memory;

public:
    void init();
    void loop();
private:
    ModbusMessage readHoldRegister(ModbusMessage request);
    ModbusMessage writeHoldRegister(ModbusMessage request);
    ModbusMessage writeMultipleRegisters(ModbusMessage request);

    float inverterFieldValue(std::shared_ptr<InverterAbstract> inv, ChannelType_t type, ChannelNum_t channel, FieldId_t field, float factor = 1.0);
    uint8_t determinePhasesCount(std::shared_ptr<InverterAbstract> inv);
    float calcDcPowerChannelSum(std::shared_ptr<InverterAbstract> inv);
    float calcDcVoltageChannelAverage(std::shared_ptr<InverterAbstract> inv);
    void initDeviceModel(uint8_t unitId);
    void updateReadResponse(ModbusMessage& response, uint8_t unitId, uint16_t addr, uint16_t words);
    ModbusMessage error(String message, uint8_t unitId, ModbusMessage request, Error errorCode, ModbusMessage response);
    void writeRegister(uint8_t unitId, uint16_t addr, uint16_t value);
};

extern SunspecApiClass SunspecApi;
