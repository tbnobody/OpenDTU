// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Holger-Steffen Stapf
 */
#include <powermeter/modbus/udp/victron/Provider.h>
#include <Arduino.h>
#include <WiFiUdp.h>
#include <LogHelper.h>

#undef TAG
static const char* TAG = "powerMeter";
static const char* SUBTAG = "Modbus/UDP/Victron";

namespace PowerMeters::Modbus::Udp::Victron {

static constexpr unsigned int modbusPort = 502;  // local port to listen on

// we only send one request which spans all registers we want to read
static constexpr uint16_t sTransactionId = 0xDEAD; // arbitrary value
static constexpr uint8_t sUnitId = 0x01;
static constexpr uint8_t sFunctionCode = 0x03; // read holding registers
static constexpr uint16_t sRegisterAddress = 0x3032;
static constexpr uint16_t sRegisterCount = 0x005A;
static constexpr uint16_t sMinPollingIntervalMs = 100;

static WiFiUDP VictronUdp;

Provider::Provider(PowerMeterUdpVictronConfig const& cfg)
    : _cfg(cfg)
{
}

bool Provider::init()
{
    VictronUdp.begin(modbusPort);
    return true;
}

Provider::~Provider()
{
    VictronUdp.stop();
}

void Provider::sendModbusRequest()
{
    auto interval = _cfg.PollingIntervalMs < sMinPollingIntervalMs
        ? sMinPollingIntervalMs
        : _cfg.PollingIntervalMs;

    uint32_t currentMillis = millis();

    if (currentMillis - _lastRequest < interval) { return; }

    std::vector<uint8_t> payload;

    payload.push_back(sTransactionId >> 8);
    payload.push_back(sTransactionId & 0xFF);

    // protocol ID
    payload.push_back(0x00);
    payload.push_back(0x00);

    // length
    payload.push_back(0x00);
    payload.push_back(0x06);

    payload.push_back(sUnitId);
    payload.push_back(sFunctionCode);
    payload.push_back(sRegisterAddress >> 8);
    payload.push_back(sRegisterAddress & 0xFF);
    payload.push_back(sRegisterCount >> 8);
    payload.push_back(sRegisterCount & 0xFF);

    VictronUdp.beginPacket(_cfg.IpAddress, modbusPort);
    VictronUdp.write(payload.data(), payload.size());
    VictronUdp.endPacket();

    _lastRequest = currentMillis;

    DTU_LOGD("sent modbus request");
}

static float readInt16(uint8_t const** buffer, uint8_t factor)
{
    uint8_t const* p = *buffer;
    int16_t value = (p[0] << 8) | p[1];
    *buffer += 2;
    return static_cast<float>(value) / factor;
}

static float readInt32(uint8_t const** buffer, uint8_t factor)
{
    uint8_t const* p = *buffer;
    int32_t value = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    *buffer += 4;
    return static_cast<float>(value) / factor;
}

static float readUint32(uint8_t const** buffer, uint8_t factor)
{
    uint8_t const* p = *buffer;
    uint32_t value = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    *buffer += 4;
    return static_cast<float>(value) / factor;
}

void Provider::parseModbusResponse()
{
    int packetSize = VictronUdp.parsePacket();
    if (0 == packetSize) { return; }

    std::vector<uint8_t> buffer(packetSize);
    VictronUdp.read(buffer.data(), packetSize);

    uint8_t const* p = buffer.data();

    DTU_LOGD("received %d bytes", packetSize);
    LogHelper::dumpBytes(TAG, SUBTAG, buffer.data(), packetSize);

    uint16_t length = 0;
    uint16_t protocolId = 0;

    auto dataRemains = [&buffer, &p](size_t amount) -> bool {
        return p - buffer.data() <= buffer.size() - amount;
    };

    // even if length is 0, we expect at least 6 bytes (the header)
    while (dataRemains(6)) {
        uint16_t currentTransactionId = (p[0] << 8) | p[1];
        p += 2;

        protocolId = (p[0] << 8) | p[1];
        p += 2;

        length = (p[0] << 8) | p[1];
        p += 2;

        if (!dataRemains(length)) {
            DTU_LOGE("unexpected end of packet");
            return;
        }

        if (currentTransactionId == sTransactionId) { break; }

        DTU_LOGI("skipping message with unexpected transaction ID: %04X", currentTransactionId);
        p += length;
    }

    if (protocolId != 0x0000) {
        DTU_LOGE("invalid protocol ID: %04X", protocolId);
        return;
    }

    uint16_t expectedLength = (sRegisterCount * 2) + 3;
    if (length != expectedLength) {
        DTU_LOGE("unexpected length: %04X, expected %04X", length, expectedLength);
        return;
    }

    uint8_t unitId = p[0];
    p += 1;

    if (unitId != sUnitId) {
        DTU_LOGE("unexpected unit ID: %02X, expected %02X", unitId, sUnitId);
        return;
    }

    uint8_t functionCode = p[0];
    p += 1;

    if (functionCode != sFunctionCode) {
        DTU_LOGE("unexpected function code: %02X, expected %02X", functionCode, sFunctionCode);
        return;
    }

    uint8_t byteCount = p[0];
    p += 1;

    uint8_t expectedByteCount = sRegisterCount * 2;
    if (byteCount != expectedByteCount) {
        DTU_LOGE("unexpected byte count: %02X, expected %02X", byteCount, expectedByteCount);
        return;
    }

    using Label = ::PowerMeters::DataPointLabel;

    auto scopedLock = _dataCurrent.lock();

    p += 2; // skip register 0x3032 (AC frequency)
    p += 2; // skip register 0x3033 (PEN voltage)

    float importEnergy = readUint32(&p, 100); // 0x3034f
    float exportEnergy = readUint32(&p, 100); // 0x3036f
    _dataCurrent.add<Label::Import>(importEnergy);
    _dataCurrent.add<Label::Export>(exportEnergy);
    p += 16; // jump to register 0x3040
    float voltageL1 = readInt16(&p, 100); // 0x3040
    float currentL1 = readInt16(&p, 100); // 0x3041
    _dataCurrent.add<Label::VoltageL1>(voltageL1);
    _dataCurrent.add<Label::CurrentL1>(currentL1);
    p += 12; // jump to register 0x3048
    float voltageL2 = readInt16(&p, 100); // 0x3048
    float currentL2 = readInt16(&p, 100); // 0x3049
    _dataCurrent.add<Label::VoltageL2>(voltageL2);
    _dataCurrent.add<Label::CurrentL2>(currentL2);
    p += 12; // jump to register 0x3050
    float voltageL3 = readInt16(&p, 100); // 0x3050
    float currentL3 = readInt16(&p, 100); // 0x3051
    _dataCurrent.add<Label::VoltageL3>(voltageL3);
    _dataCurrent.add<Label::CurrentL3>(currentL3);
    p += 92; // jump from 0x3052 to 0x3080 (0x2E registers = 92 bytes)
    float powerTotal = readInt32(&p, 1); // 0x3080f
    float powerL1 = readInt32(&p, 1); // 0x3082f
    _dataCurrent.add<Label::PowerTotal>(powerTotal);
    _dataCurrent.add<Label::PowerL1>(powerL1);
    p += 4; // jump to 0x3086
    float powerL2 = readInt32(&p, 1); // 0x3086f
    _dataCurrent.add<Label::PowerL2>(powerL2);
    p += 4; // jump to 0x308A
    float powerL3 = readInt32(&p, 1); // 0x308Af
    _dataCurrent.add<Label::PowerL3>(powerL3);

    DTU_LOGD("Import=%.3f, Export=%.3f, "
             "V1=%.2f, I1=%.2f, P1=%.2f, "
             "V2=%.2f, I2=%.2f, P2=%.2f, "
             "V3=%.2f, I3=%.2f, P3=%.2f, "
             "Ptotal=%.2f",
             importEnergy, exportEnergy,
             voltageL1, currentL1, powerL1,
             voltageL2, currentL2, powerL2,
             voltageL3, currentL3, powerL3,
             powerTotal);
}

void Provider::loop()
{
    sendModbusRequest();
    parseModbusResponse();
}

} // namespace PowerMeters::Modbus::Udp::Victron
