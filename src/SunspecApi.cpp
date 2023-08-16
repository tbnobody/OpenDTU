#include "SunspecApi.h"
#include "MessageOutput.h"

ModbusServerTCPasync mb;
SunspecApiClass SunspecApi;

SunspecModel::SunspecModel(std::map<uint16_t, uint16_t>& registers, uint8_t unitId, uint16_t modelId, uint16_t startAddr)
    : _registers(registers)
    , _unitId(unitId)
    , _startAddr(startAddr)
{
    _buffer.push_back(modelId);
    _buffer.push_back(0);
}

SunspecModel::~SunspecModel()
{
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

SunspecModel& SunspecModel::add(uint16_t value)
{
    _buffer.push_back(value);
    return *this;
}

SunspecModel& SunspecModel::addUInt32(uint32_t value)
{
    auto p = reinterpret_cast<uint16_t const*>(&value);
    _buffer.push_back(p[0]); // XXX - check endianess
    _buffer.push_back(p[1]);
    return *this;
}

SunspecModel& SunspecModel::add(String utf8value, unsigned int regCount)
{
    if (utf8value.length() > regCount * 2) {
        utf8value = utf8value.substring(0, FindUtf8CutPosition(utf8value, regCount * 2));
    }

    auto len = utf8value.length();
    for (int i = 0; i < regCount; i++) {
        uint8_t a = (i * 2 < len) ? utf8value.charAt(i * 2) : 0;
        uint8_t b = (i * 2 + 1 < len) ? utf8value.charAt(i * 2 + 1) : 0;
        _buffer.push_back(a << 8 | b);
    }
    return *this;
}

SunspecModel& SunspecModel::addFloat(float value)
{
    auto p = reinterpret_cast<uint16_t const*>(&value);
    _buffer.push_back(p[1]);
    _buffer.push_back(p[0]);
    return *this;
}

uint16_t SunspecModel::commit()
{
    auto bufferSize = _buffer.size();
    _buffer[1] = bufferSize - 2; // model length
    for (auto i = 0; i < bufferSize; i++) {
        _registers[_startAddr + i] = _buffer[i];
    }

    return _startAddr + bufferSize;
}

void SunspecApiClass::init()
{
    updateAllRegisters();

    auto invCount = Hoymiles.getNumInverters();
    for (uint8_t i = 0; i < invCount; i++) {
        auto unitId = SunspecConstants::BASE_UNIT_ID + i;
        mb.registerWorker(unitId, READ_HOLD_REGISTER, std::bind(&SunspecApiClass::FC03, this, std::placeholders::_1));
    }

    mb.start(502, 10, 10000);
}

void SunspecApiClass::updateAllRegisters()
{
    MessageOutput.println("SunspecApiClass::updateRegisters for all unit IDs");

    auto invCount = Hoymiles.getNumInverters();
    for (uint8_t i = 0; i < invCount; i++) {
        auto unitId = SunspecConstants::BASE_UNIT_ID + i;
        updateRegisters(unitId);
    }
}

void SunspecApiClass::loop()
{
    if (millis() - _lastLoopMillis > UPDATE_INTERVAL_MILLIS) {
        _lastLoopMillis = millis();
        updateAllRegisters();
        MessageOutput.println("free heap: " +  String(ESP.getFreeHeap()));
    }
}

ModbusMessage SunspecApiClass::FC03(ModbusMessage request)
{
    ModbusMessage response;
    uint8_t unitId = request.getServerID(); // The unit ID of the request
    uint16_t addr = 0; // Start address
    uint16_t words = 0; // # of words (16 bit registers) requested
    request.get(2, addr); // read address from request
    request.get(4, words); // read # of words from request

    MessageOutput.println("SunspecApiClass::FC03 request for unit " + String(unitId) + ", addr " + String(addr) + ", words " + String(words));

    if (unitId < 1 || unitId > SunspecConstants::BASE_UNIT_ID + Hoymiles.getNumInverters() - 1) {
        MessageOutput.println("SunspecApiClass::FC03 - invalid unit ID " + unitId);
        response.setError(unitId, request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
        return response;
    }

    // Request for FC 0x03?
    if (request.getFunctionCode() != READ_HOLD_REGISTER) {
        MessageOutput.println("SunspecApiClass::FC03 - unsupported function code " + request.getFunctionCode());
        response.setError(unitId, request.getFunctionCode(), ILLEGAL_FUNCTION);
        return response;
    }

    updateResponse(response, unitId, addr, words);
    return response;
}

void SunspecApiClass::sunspecHeader(uint8_t unitId, uint16_t startAddr)
{
    _memory[unitId][startAddr] = 0x5375; // Sunspec start marker ("Su")
    _memory[unitId][startAddr + 1] = 0x6e53; // Sunspec start marker ("nS")
    _memory[unitId][startAddr + 2] = 0xFFFF; // Sunspec end marker, to be overridden later
    _memory[unitId][startAddr + 3] = 0;
}

SunspecModel SunspecApiClass::newModel(uint8_t unitId, uint16_t modelId, uint16_t startAddr)
{
    return SunspecModel(_memory[unitId], unitId, modelId, startAddr);
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

void SunspecApiClass::updateRegisters(uint8_t unitId)
{
    //MessageOutput.println("SunspecApiClass::updateRegisters for unit ID " + String(unitId));

    // Sun Spec Scale Factors
    const int16_t acCurrentScaleFactor = -2;
    const int16_t acVoltageScaleFactor = -1;
    const int16_t acPowerScaleFactor = -1;
    const int16_t acFrequencyScaleFactor = -2;
    const int16_t acReactivePowerScaleFactor = -1;

    auto inv = Hoymiles.getInverterByPos(unitId - SunspecConstants::BASE_UNIT_ID);

    char version[16];
    snprintf(version, sizeof(version), "%d.%d.%d", CONFIG_VERSION >> 24 & 0xff, CONFIG_VERSION >> 16 & 0xff, CONFIG_VERSION >> 8 & 0xff);

    sunspecHeader(unitId, SunspecConstants::COMMON_BASE_ADDRESS);

    // Sunspec Common Registers (Model 1)
    newModel(unitId, 1, (uint16_t)(SunspecConstants::COMMON_BASE_ADDRESS + 2))
        .add("Hoymiles", 16) // Manifacturer
        .add("OpenDTU", 16) // Model
        .add("Sunspec", 8) // Option
        .add(version, 8) // Firmware version
        .add(inv->serialString(), 16) // Serial Number
        .add(unitId) // Device Address (?)
        .add(0x8000) // Pad
        .commit();

    // Sunspec Single Phase Inverter (Model 101)
    uint8_t phasesCount = determinePhasesCount(inv);
    float acCurrentTotal = inverterFieldValue(inv, TYPE_AC, CH0, FLD_IAC);
    float acReactivePower = inverterFieldValue(inv, TYPE_AC, CH0, FLD_Q);
    float acCurrentPhase1 = inverterFieldValue(inv, TYPE_AC, CH0, FLD_IAC_1);
    float acCurrentPhase2 = inverterFieldValue(inv, TYPE_AC, CH0, FLD_IAC_2);
    float acCurrentPhase3 = inverterFieldValue(inv, TYPE_AC, CH0, FLD_IAC_3);
    float acVoltagePhaseToN1 = inverterFieldValue(inv, TYPE_AC, CH0, FLD_UAC_1N);
    float acVoltagePhaseToN2 = inverterFieldValue(inv, TYPE_AC, CH0, FLD_UAC_2N);
    float acVoltagePhaseToN3 = inverterFieldValue(inv, TYPE_AC, CH0, FLD_UAC_3N);
    float acPower = inverterFieldValue(inv, TYPE_AC, CH0, FLD_PAC);
    float acFrequency = inverterFieldValue(inv, TYPE_AC, CH0, FLD_F);
    float acPowerFactor = inverterFieldValue(inv, TYPE_AC, CH0, FLD_PF);
    float acLifetimeEnergy = inverterFieldValue(inv, TYPE_AC, CH0, FLD_YT, 1000);
    float dcVoltage = calcDcVoltageChannelAverage(inv);
    float dcPower = calcDcPowerChannelSum(inv);
    float dcCurrent = dcVoltage == 0 ? 0 : (dcPower / dcVoltage);
    float temperature = inverterFieldValue(inv, TYPE_INV, CH0, FLD_T);
    float powerLimit = inv->SystemConfigPara()->getLimitPercent();

    newModel(unitId, 100 + phasesCount, (uint16_t)(SunspecConstants::COMMON_BASE_ADDRESS + 70))
        .add((acCurrentTotal * pow(10, -acCurrentScaleFactor)))
        .add(isnan(acCurrentPhase1) ? 0x8000 : (uint16_t)acCurrentPhase1 * pow(10, -acCurrentScaleFactor))
        .add(isnan(acCurrentPhase2) ? 0x8000 : (uint16_t)acCurrentPhase2 * pow(10, -acCurrentScaleFactor))
        .add(isnan(acCurrentPhase3) ? 0x8000 : (uint16_t)acCurrentPhase3 * pow(10, -acCurrentScaleFactor))
        .add(acCurrentScaleFactor) // 40076
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(isnan(acVoltagePhaseToN1) ? 0x8000 : (uint16_t)acVoltagePhaseToN1 * pow(10, -acVoltageScaleFactor))
        .add(isnan(acVoltagePhaseToN2) ? 0x8000 : (uint16_t)acVoltagePhaseToN2 * pow(10, -acVoltageScaleFactor))
        .add(isnan(acVoltagePhaseToN3) ? 0x8000 : (uint16_t)acVoltagePhaseToN3 * pow(10, -acVoltageScaleFactor))
        .add(acVoltageScaleFactor) // 40083
        .add((acPower * pow(10, -acPowerScaleFactor)))
        .add(acPowerScaleFactor)
        .add((acFrequency * pow(10, -acFrequencyScaleFactor)))
        .add(acFrequencyScaleFactor)
        .add(0x8000)
        .add(0x8000)
        .add((acReactivePower * pow(10, -acReactivePowerScaleFactor)))
        .add(acReactivePowerScaleFactor)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .commit();

    // Sunspec Single Phase Inverter (Model 111)
    newModel(unitId, 110 + phasesCount, (uint16_t)(SunspecConstants::COMMON_BASE_ADDRESS + 110))
        .addFloat(acCurrentTotal)
        .addFloat(acCurrentPhase1)
        .addFloat(acCurrentPhase2)
        .addFloat(acCurrentPhase3)
        .addFloat(NAN) // Phase Voltage 12
        .addFloat(NAN) // Phase Voltage 23
        .addFloat(NAN) // Phase Voltage 31
        .addFloat(acVoltagePhaseToN1)
        .addFloat(acVoltagePhaseToN2)
        .addFloat(acVoltagePhaseToN3)
        .addFloat(acPower)
        .addFloat(acFrequency)
        .addFloat(NAN) // VA
        .addFloat(acReactivePower)
        .addFloat(acPowerFactor)
        .addFloat(acLifetimeEnergy) // Wh
        .addFloat(dcCurrent) // DC Amps
        .addFloat(dcVoltage) // DC Voltage
        .addFloat(dcPower) // DC Watts
        .addFloat(temperature) // 40 Cabinet Temperature
        .addFloat(temperature) // 42 Heat Sink Temperature
        .addFloat(temperature) // 44 Transformer Temperature
        .addFloat(temperature) // 46 Other Temperature
        .add(0x8000) // 48 Operating State
        .add(0x8000) // 49 Vendor Operating State
        .addUInt32(0) // 50 Event 1
        .addUInt32(0) // 52 Event Bitfield 2
        .addUInt32(0) // 54 Vendor Event Bitfield 1
        .addUInt32(0) // 56 Vendor Event Bitfield 2
        .addUInt32(0) // 58 Vendor Event Bitfield 3
        .addUInt32(0) // 60 Vendor Event Bitfield 4
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .add(0x8000)
        .commit();

    // Sunspec Immediate Controls (Model 123)
    auto nextAddr = newModel(unitId, 123, (uint16_t)(SunspecConstants::COMMON_BASE_ADDRESS + 184))
                        .add(0x8000)
                        .add(0x8000)
                        .add(0x8000)
                        .add(powerLimit) // 40189
                        .add(0x8000)
                        .add(0x8000)
                        .add(0x8000)
                        .add(powerLimit >= 0 && powerLimit < 100) // 40193
                        .commit();

    // Sunspec end marker
    newModel(unitId, 0xFFFF, nextAddr).commit();
}

void SunspecApiClass::updateResponse(ModbusMessage& response, uint8_t unitId, uint16_t addr, uint16_t words)
{
    auto it = _memory.find(unitId);
    if (it == _memory.end()) {
        MessageOutput.println("SunspecApiClass::updateResponse - invalid unit ID " + String(unitId));
        response.setError(unitId, READ_HOLD_REGISTER, ILLEGAL_DATA_ADDRESS);
        return;
    }

    auto registers = it->second;
    auto baseAddr = registers.begin()->first;
    auto lastAddr = registers.rbegin()->first;

    if (addr < baseAddr || addr + words - 1 > lastAddr) {
        MessageOutput.println("SunspecApiClass::updateResponse - address out of range [" + String(baseAddr) + ", " + String(lastAddr) + "]");
        response.setError(unitId, READ_HOLD_REGISTER, ILLEGAL_DATA_ADDRESS);
        return;
    }

    // setup response
    response.add(unitId, READ_HOLD_REGISTER, (uint8_t)(words * 2));
    for (auto i = 0; i < words; i++) {
        auto it = registers.find(addr + i);
        if (it != registers.end()) {
            response.add(it->second);
        } else {
            response.add(0x8000);
        }
    }
}