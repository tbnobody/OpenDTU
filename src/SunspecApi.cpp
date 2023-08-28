#include "SunspecApi.h"
#include "MessageOutput.h"

ModbusServerTCPasync mb;
SunspecApiClass SunspecApi;

float bound(float lower, float value, float upper)
{
    if (value < lower) {
        return lower;
    }
    if (value > upper) {
        return upper;
    }
    return value;
}

size_t FindUtf8CutPosition(const String& str, size_t max_size)
{
    if (str.length() <= max_size) {
        return str.length();
    }
    for (int pos = max_size - 1; pos >= 0; pos--) {
        unsigned char byte = static_cast<unsigned char>(str[pos]);
        if ((byte & 0xC0) != 0x80) {
            return pos;
        }
    }
    // first byte isn't a valid UTF-8 starting point
    throw std::invalid_argument("not a valid UTF-8 string");
}

namespace Modbus
{

    uint16_t UInt32Value::getValue(uint16_t offset) const {
        auto p = reinterpret_cast<uint16_t const*>(&_value);
        if (offset == 0) {
            return p[1];
        }
        if (offset == 1) {
            return p[0];
        }
        return 0xFFFF;
    }

    uint16_t Float32Value::getValue(uint16_t offset) const {
        auto p = reinterpret_cast<uint16_t const*>(&_value);
        if (offset == 0) {
            return p[1];
        }
        if (offset == 1) {
            return p[0];
        }
        return 0xFFFF;
    }

    uint16_t StringValue::getValue(uint16_t offset) const {
        if (offset >= _regCount) {
            return 0xFFFF;
        }
        auto len = std::min(FindUtf8CutPosition(_value, _regCount * 2), _value.length());
        uint8_t a = (offset * 2 < len) ? _value.charAt(offset * 2) : 0;
        uint8_t b = (offset * 2 + 1 < len) ? _value.charAt(offset * 2 + 1) : 0;
        return a << 8 | b;
    }

    uint16_t FloatCallback::getValue(uint16_t offset) const {
        auto value = _currentValue;
        auto p = reinterpret_cast<uint16_t const*>(&value);
        if (offset == 0) {
            return p[1];
        }
        if (offset == 1) {
            return p[0];
        }
        return 0xFFFF;
    }

    uint16_t CompositeValue::getValue(uint16_t offset) const {
        uint16_t regCount = 0;
        for (const auto& value : _values) {
            if (offset < regCount + value->regCount()) {
                auto valueOffset = offset - regCount;
                if (valueOffset == 0) {
                    // trigger callback
                    value->update();
                }
                return value->getValue(valueOffset);
            }
            regCount += value->regCount();
        }
        return 0xFFFF;
    }

    uint16_t CompositeValue::regCount() const {
        uint16_t regCount = 0;
        for (const auto& value : _values) {
            regCount += value->regCount();
        }
        return regCount;
    }

} // namespace Modbus

void SunspecApiClass::init()
{
    auto invCount = Hoymiles.getNumInverters();
    for (uint8_t i = 0; i < invCount; i++) {
        auto unitId = SunspecConstants::BASE_UNIT_ID + i;
        initDeviceModel(unitId);
        mb.registerWorker(unitId, READ_HOLD_REGISTER, std::bind(&SunspecApiClass::readHoldRegister, this, std::placeholders::_1));
        mb.registerWorker(unitId, WRITE_HOLD_REGISTER, std::bind(&SunspecApiClass::writeHoldRegister, this, std::placeholders::_1));
        mb.registerWorker(unitId, WRITE_MULT_REGISTERS, std::bind(&SunspecApiClass::writeMultipleRegisters, this, std::placeholders::_1));
    }

    mb.start(502, 10, 10000);
}

void SunspecApiClass::loop()
{
    if (millis() - _lastLoopMillis > UPDATE_INTERVAL_MILLIS) {
        _lastLoopMillis = millis();
        MessageOutput.println("Free heap: " + String(ESP.getFreeHeap()));
    }
}

ModbusMessage SunspecApiClass::readHoldRegister(ModbusMessage request)
{
    ModbusMessage response;
    uint8_t unitId = request.getServerID(); // The unit ID of the request
    uint16_t addr = 0; // Start address
    uint16_t words = 0; // # of words (16 bit registers) requested
    request.get(2, addr); // read address from request
    request.get(4, words); // read # of words from request

    MessageOutput.println("Sunspec: readHoldRegister request for unit " + String(unitId) + ", addr " + String(addr) + ", words " + String(words));

    if (unitId < 1 || unitId > SunspecConstants::BASE_UNIT_ID + Hoymiles.getNumInverters() - 1) {
        return error("Sunspec: readHoldRegister - invalid unit ID " + unitId, unitId, request, ILLEGAL_DATA_ADDRESS, response);
    }

    // Request for FC 0x03?
    if (request.getFunctionCode() != READ_HOLD_REGISTER) {
        return error("Sunspec: readHoldRegister - unsupported function code " + request.getFunctionCode(), unitId, request, ILLEGAL_FUNCTION, response);
    }

    updateReadResponse(response, unitId, addr, words);
    return response;
}

ModbusMessage SunspecApiClass::writeHoldRegister(ModbusMessage request)
{
    ModbusMessage response;
    uint8_t unitId = request.getServerID(); // The unit ID of the request
    uint16_t addr = 0; // Start address
    uint16_t value = 0; // Value to write
    request.get(2, addr); // read address from request
    request.get(4, value); // read value from request

    MessageOutput.println("Sunspec: writeHoldRegister request for unit " + String(unitId) + ", addr " + String(addr) + ", value " + String(value));

    if (unitId < 1 || unitId > SunspecConstants::BASE_UNIT_ID + Hoymiles.getNumInverters() - 1) {
        return error("Sunspec: writeHoldRegister - invalid unit ID " + unitId, unitId, request, ILLEGAL_DATA_ADDRESS, response);
    }

    // Request for FC 0x06?
    if (request.getFunctionCode() != WRITE_HOLD_REGISTER) {
        return error("Sunspec: writeHoldRegister - unsupported function code " + request.getFunctionCode(), unitId, request, ILLEGAL_FUNCTION, response);
    }

    writeRegister(unitId, addr, value);
    response.setServerID(request.getServerID());
    response.setFunctionCode(request.getFunctionCode());
    response.add(addr);
    response.add(value);
    return response;
}

ModbusMessage SunspecApiClass::writeMultipleRegisters(ModbusMessage request)
{
    ModbusMessage response;
    uint8_t unitId = request.getServerID(); // The unit ID of the request
    uint16_t addr = 0; // Start address
    uint16_t words = 0; // # of words (16 bit registers) requested
    uint8_t byteCount = 0; // # of bytes requested
    request.get(2, addr); // read address from request
    request.get(4, words); // read # of words from request
    request.get(6, byteCount); // read # of bytes from request

    MessageOutput.println("Sunspec: writeMultipleRegisters request for unit " + String(unitId) + ", addr " + String(addr) + ", words " + String(words) + ", byteCount " + String(byteCount));

    if (unitId < 1 || unitId > SunspecConstants::BASE_UNIT_ID + Hoymiles.getNumInverters() - 1) {
        return error("Sunspec: writeMultipleRegisters - invalid unit ID " + unitId, unitId, request, ILLEGAL_DATA_ADDRESS, response);
    }

    if (byteCount != words * 2) {
        return error("Sunspec: writeMultipleRegisters - byteCount doesn't match number of registers to read", unitId, request, ILLEGAL_DATA_ADDRESS, response);
    }

    // Request for FC 0x10?
    if (request.getFunctionCode() != WRITE_MULT_REGISTERS) {
        return error("Sunspec: writeMultipleRegisters - unsupported function code " + request.getFunctionCode(), unitId, request, ILLEGAL_FUNCTION, response);
    }

    for (uint16_t i = 0; i < words; i++) {
        uint16_t value;
        request.get(7 + i * 2, value); // read value from request

        writeRegister(unitId, addr + i, value);
    }

    response.setServerID(request.getServerID());
    response.setFunctionCode(request.getFunctionCode());
    response.add(addr);
    response.add(words);
    response.add(byteCount);
    return response;
}

float SunspecApiClass::inverterFieldValue(std::shared_ptr<InverterAbstract> inv, ChannelType_t type, ChannelNum_t channel, FieldId_t field, float factor)
{
    auto stats = inv->Statistics();
    if (!stats->hasChannelFieldValue(type, channel, field)) {
        return NAN;
    }
    return stats->getChannelFieldValue(type, channel, field) * factor;
}

uint8_t SunspecApiClass::determinePhasesCount(std::shared_ptr<InverterAbstract> inv)
{
    auto stats = inv->Statistics();
    if (stats->hasChannelFieldValue(TYPE_AC, CH0, FLD_UAC_3N) || stats->hasChannelFieldValue(TYPE_AC, CH0, FLD_IAC_3)) {
        return 3;
    }
    if (stats->hasChannelFieldValue(TYPE_AC, CH0, FLD_UAC_2N) || stats->hasChannelFieldValue(TYPE_AC, CH0, FLD_IAC_2)) {
        return 2;
    }
    return 1;
}

float SunspecApiClass::calcDcPowerChannelSum(std::shared_ptr<InverterAbstract> inv)
{
    auto stats = inv->Statistics();
    float result = 0;
    for (auto& channel : stats->getChannelsByType(TYPE_DC)) {
        result += stats->getChannelFieldValue(TYPE_DC, channel, FLD_PDC);
    }
    return result;
}

float SunspecApiClass::calcDcVoltageChannelAverage(std::shared_ptr<InverterAbstract> inv)
{
    StatisticsParser* stats = inv->Statistics();
    float sum = 0;
    uint8_t count = 0;
    for (auto& channel : stats->getChannelsByType(TYPE_DC)) {
        sum += stats->getChannelFieldValue(TYPE_DC, channel, FLD_UDC);
        count++;
    }
    return sum / count;
}

void SunspecApiClass::initDeviceModel(uint8_t unitId) {
    MessageOutput.println("Sunspec: initDeviceModel for " + String(unitId));

    auto inv = Hoymiles.getInverterByPos(unitId - SunspecConstants::BASE_UNIT_ID);

    char version[16];
    snprintf(version, sizeof(version), "%d.%d.%d", CONFIG_VERSION >> 24 & 0xff, CONFIG_VERSION >> 16 & 0xff, CONFIG_VERSION >> 8 & 0xff);

    uint8_t phasesCount = determinePhasesCount(inv);

    float acMaxPower = inv->DevInfo()->getMaxPower();

    DeviceModel deviceModel;
    
    deviceModel.addSunspecModel(1, std::make_shared<Modbus::CompositeValue>(
        Modbus::CompositeValue()
            .addString("OpenDTU", 16)
            .addString(inv->name(), 16)
            .addString(inv->typeName(), 8)
            .addString(version, 8)
            .addString(inv->serialString(), 16)
            .addUInt16(unitId)
            .addUInt16(0xFFFF)));

    // SunSpec Float Inverter Model 11x
    deviceModel.addSunspecModel(110 + phasesCount, std::make_shared<Modbus::CompositeValue>(
        Modbus::CompositeValue()
            .addFloat([this, inv](){ 
                float acCurrentTotal = inverterFieldValue(inv, TYPE_AC, CH0, FLD_IAC);
                if (isnan(acCurrentTotal) || acCurrentTotal == 0) {
                    auto i1 = inverterFieldValue(inv, TYPE_AC, CH0, FLD_IAC_1);
                    auto i2 = inverterFieldValue(inv, TYPE_AC, CH0, FLD_IAC_2);
                    auto i3 = inverterFieldValue(inv, TYPE_AC, CH0, FLD_IAC_3);
                    acCurrentTotal = (isnan(i1) ? 0 : i1) + (isnan(i2) ? 0 : i2) + (isnan(i3) ? 0 : i3);
                }
                return acCurrentTotal;
            }) // 2
            .addFloat([this, inv]() { return inverterFieldValue(inv, TYPE_AC, CH0, FLD_IAC_1); }) // 4
            .addFloat([this, inv]() { return inverterFieldValue(inv, TYPE_AC, CH0, FLD_IAC_2); }) // 6
            .addFloat([this, inv]() { return inverterFieldValue(inv, TYPE_AC, CH0, FLD_IAC_3); }) // 8
            .addFloat(NAN) // 10 Phase Voltage 12
            .addFloat(NAN) // 12 Phase Voltage 23
            .addFloat(NAN) // 14 Phase Voltage 31
            .addFloat([this, inv]() { 
                float acVoltagePhaseToN1 = inverterFieldValue(inv, TYPE_AC, CH0, FLD_UAC_1N);
                if (isnan(acVoltagePhaseToN1)) {
                    acVoltagePhaseToN1 = inverterFieldValue(inv, TYPE_AC, CH0, FLD_UAC);
                }
                return acVoltagePhaseToN1;
            }) // 16
            .addFloat([this, inv]() { return inverterFieldValue(inv, TYPE_AC, CH0, FLD_UAC_2N); }) // 18
            .addFloat([this, inv]() { return inverterFieldValue(inv, TYPE_AC, CH0, FLD_UAC_3N); }) // 20
            .addFloat([this, inv]() { return inverterFieldValue(inv, TYPE_AC, CH0, FLD_PAC); }) // 22
            .addFloat([this, inv]() { return inverterFieldValue(inv, TYPE_AC, CH0, FLD_F); }) // 24
            .addFloat(NAN) // 26 VA
            .addFloat([this, inv](){ return inverterFieldValue(inv, TYPE_AC, CH0, FLD_Q); }) // 28
            .addFloat([this, inv](){ 
                auto pf = inverterFieldValue(inv, TYPE_AC, CH0, FLD_PF);
                return (pf ? pf : 100); 
            }) // 30
            .addFloat([this, inv](){ 
                auto energy = inverterFieldValue(inv, TYPE_AC, CH0, FLD_YT, 1000);
                return (energy ? energy : NAN); 
            }) // 32 Wh
            .addFloat([this, inv](){ 
                auto dcVoltage = calcDcVoltageChannelAverage(inv); 
                return dcVoltage ? (calcDcPowerChannelSum(inv) / dcVoltage) : 0;
            }) // 34 DC Amps
            .addFloat([this, inv](){ return calcDcVoltageChannelAverage(inv); }) // 36 DC Voltage
            .addFloat([this, inv](){ return calcDcPowerChannelSum(inv); }) // 38 DC Watts
            .addFloat([this, inv](){ return inverterFieldValue(inv, TYPE_INV, CH0, FLD_T); }) // 40 Cabinet Temperature
            .addFloat([this, inv](){ return inverterFieldValue(inv, TYPE_INV, CH0, FLD_T); }) // 42 Heat Sink Temperature
            .addFloat([this, inv](){ return inverterFieldValue(inv, TYPE_INV, CH0, FLD_T); }) // 44 Transformer Temperature
            .addFloat([this, inv](){ return inverterFieldValue(inv, TYPE_INV, CH0, FLD_T); }) // 46 Other Temperature
            .addUInt16(0xFFFF) // 48 Operating State
            .addUInt16(0xFFFF) // 49 Vendor Operating State
            .addUInt32(0) // 50 Event 1
            .addUInt32(0) // 52 Event Bitfield 2
            .addUInt32(0) // 54 Vendor Event Bitfield 1
            .addUInt32(0) // 56 Vendor Event Bitfield 2
            .addUInt32(0) // 58 Vendor Event Bitfield 3
            .addUInt32(0) // 60 Vendor Event Bitfield 4
            .addUInt16(0xFFFF) // 62
            .addUInt16(0xFFFF) // 63
            .addUInt16(0xFFFF) // 64
            .addUInt16(0xFFFF) // 65
            .addUInt16(0xFFFF) // 66
            .addUInt16(0xFFFF) // 67
            .addUInt16(0xFFFF) // 68
            .addUInt16(0xFFFF) // 69
            .addUInt16(0xFFFF) // 70
            .addUInt16(0xFFFF) // 71
            .addUInt16(0xFFFF) // 72
            .addUInt16(0xFFFF) // 73
            ));

    // SunSpec Nameplate Model 120
    deviceModel.addSunspecModel(120, std::make_shared<Modbus::CompositeValue>(
        Modbus::CompositeValue()
            .addUInt16(4) // 122 DERTyp
            .addUInt16(acMaxPower * pow(10, -acPowerScaleFactor)) // 123 WRtg
            .addUInt16(acPowerScaleFactor) // 124 WRtg_SF
            .addUInt16(0xFFFF)));

    // SunSpec Immediate Controls (Model 123)
    deviceModel.addSunspecModel(123, std::make_shared<Modbus::CompositeValue>(
        Modbus::CompositeValue()
            .addUInt16(0xFFFF) // Conn_WinTms
            .addUInt16(0xFFFF) // Conn_RvrtTms
            .addUInt16([this, inv](){ return inv->isReachable(); }) // Conn: 0 or 1 (DISCONNECT/CONNECT)
            .addUInt16([this, inv](){ 
                auto lim = bound(0, inv->SystemConfigPara()->getLimitPercent(), 100);
                return (lim ? lim : 100) * pow(10, -acPowerLimitScaleFactor);
            }) // WMaxLimPct
            .addUInt16(0xFFFF) // WMaxLimPct_WinTms
            .addUInt16(0xFFFF) // WMaxLimPct_RvrtTms
            .addUInt16(0xFFFF) // WMaxLimPct_RmpTms
            .addUInt16([this, inv](){ 
                auto lim = inv->SystemConfigPara()->getLimitPercent(); 
                return (lim > 0) && (lim < 100); 
            }) // WMaxLimPct_Ena (0 or 1)
            .addUInt16([this, inv](){ 
                auto pf = inverterFieldValue(inv, TYPE_AC, CH0, FLD_PF);
                return (pf ? pf : 100) * pow(10, -acPowerFactorScaleFactor); 
            }) // OutPFSet
            .addUInt16(0xFFFF) // OutPFSet_WinTms
            .addUInt16(0xFFFF) // OutPFSet_RvrtTms
            .addUInt16(0xFFFF) // OutPFSet_RmpTms
            .addUInt16(0) //OutPFSet_Ena (0 or 1)
            .addUInt16(0xFFFF) // VArWMaxPct
            .addUInt16(0xFFFF) // VArMaxPct
            .addUInt16(0xFFFF) // VArAvalPct
            .addUInt16(0xFFFF) // VArPct_WinTms
            .addUInt16(0xFFFF) // VArPct_RvrtTms
            .addUInt16(0xFFFF) // VArPct_RmpTms
            .addUInt16(0xFFFF) // VArPct_Mod
            .addUInt16(0) // VArPct_Ena (0 or 1)
            .addInt16(acPowerLimitScaleFactor) // WMaxLimPct_SF
            .addInt16(acPowerFactorScaleFactor) // OutPFSet_SF
            .addUInt16(0xFFFF)));

    _memory[unitId] = deviceModel.build();
}

void SunspecApiClass::updateReadResponse(ModbusMessage& response, uint8_t unitId, uint16_t addr, uint16_t words)
{
    auto it = _memory.find(unitId);
    if (it == _memory.end()) {
        MessageOutput.println("Sunspec: updateReadResponse - invalid unit ID " + String(unitId));
        response.setError(unitId, READ_HOLD_REGISTER, ILLEGAL_DATA_ADDRESS);
        return;
    }

    auto deviceModel = it->second;
    auto baseAddr = SunspecConstants::COMMON_BASE_ADDRESS;
    auto lastAddr = baseAddr + deviceModel->regCount() - 1;

    if (addr < baseAddr || addr + words - 1 > lastAddr) {
        MessageOutput.println("Sunspec: updateReadResponse - address out of range [" + String(baseAddr) + ", " + String(lastAddr) + "]");
        response.setError(unitId, READ_HOLD_REGISTER, ILLEGAL_DATA_ADDRESS);
        return;
    }

    response.add(unitId, READ_HOLD_REGISTER, (uint8_t)(words * 2));
    for (auto i = 0; i < words; i++) {
        response.add(deviceModel->getValue(addr - baseAddr + i));
    }
}

ModbusMessage SunspecApiClass::error(String message, uint8_t unitId, ModbusMessage request, Error errorCode, ModbusMessage response)
{
    MessageOutput.println(message);
    response.setError(unitId, request.getFunctionCode(), errorCode);
    return response;
}

void SunspecApiClass::writeRegister(uint8_t unitId, uint16_t addr, uint16_t value) {
    MessageOutput.print("SunSpec: writing value " + String(value) + " to register " + String(addr));

    auto inv = Hoymiles.getInverterByPos(unitId - SunspecConstants::BASE_UNIT_ID);

    switch (addr) {
    {
        case SunspecConstants::INVERTER_MODEL_IMMEDIATE_CONTROLS_BASE_ADDRESS + 5: {
            MessageOutput.print(": set power-limit");
            float acPowerLimitPct = bound(2, value / pow(10, -acPowerLimitScaleFactor), 100); // 2 to 100 (Inverter does not support values < 2%)
            _powerLimitPct = acPowerLimitPct;
            if (inv->SystemConfigPara()->getLimitPercent() < 100) {
                // If power-limit is already enabled, update the limit
                inv->SystemConfigPara()->setLimitPercent(acPowerLimitPct);
            }
            break;
        }

        case SunspecConstants::INVERTER_MODEL_IMMEDIATE_CONTROLS_BASE_ADDRESS + 14: {
            MessageOutput.print(": power-limit enable/disable");
            inv->SystemConfigPara()->setLimitPercent(value ? _powerLimitPct : 100);
            break;
        }

        default:
            MessageOutput.print(": unsupported, skipped");
        }
    }
    MessageOutput.println();
}
