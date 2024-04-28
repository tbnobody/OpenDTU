#include "SerialModbusRTU.h"

#include <vector>

namespace {
constexpr size_t MODBUS_ADU = 256;

// Function codes
constexpr uint8_t FC_READ_COILS = 0x01; // Read Coils (Output) Status
constexpr uint8_t FC_READ_INPUT_STAT = 0x02; // Not implemented. Read Input Status (Discrete Inputs)
constexpr uint8_t FC_READ_REGS = 0x03; // Read Holding Registers
constexpr uint8_t FC_READ_INPUT_REGS = 0x04; // Not implemented. Read Input Registers
constexpr uint8_t FC_WRITE_COIL = 0x05; // Write Single Coil (Output)
constexpr uint8_t FC_WRITE_REG = 0x06; // Not implemented. Preset Single Register
constexpr uint8_t FC_DIAGNOSTICS = 0x08; // Not implemented. Diagnostics (Serial Line only)
constexpr uint8_t FC_WRITE_COILS = 0x0F; // Not implemented. Write Multiple Coils (Outputs)
constexpr uint8_t FC_WRITE_REGS = 0x10; // Write block of contiguous registers
constexpr uint8_t FC_READ_FILE_REC = 0x14; // Not implemented. Read File Record
constexpr uint8_t FC_WRITE_FILE_REC = 0x15; // Not implemented. Write File Record
constexpr uint8_t FC_MASKWRITE_REG = 0x16; // Not implemented. Mask Write Register
constexpr uint8_t FC_READWRITE_REGS = 0x17; // Not implemented. Read/Write Multiple registers

// Code from https://github.com/LacobusVentura/MODBUS-CRC16
// Copyright (c) 2019 Tiago Ventura
uint16_t computeModbusCRC(const uint8_t* buf, size_t len)
{
    constexpr uint16_t table[256] = {
        0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241, 0xC601, 0x06C0, 0x0780, 0xC741, 0x0500,
        0xC5C1, 0xC481, 0x0440, 0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40, 0x0A00, 0xCAC1,
        0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841, 0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81,
        0x1A40, 0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41, 0x1400, 0xD4C1, 0xD581, 0x1540,
        0xD701, 0x17C0, 0x1680, 0xD641, 0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040, 0xF001,
        0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240, 0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0,
        0x3480, 0xF441, 0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41, 0xFA01, 0x3AC0, 0x3B80,
        0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840, 0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
        0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40, 0xE401, 0x24C0, 0x2580, 0xE541, 0x2700,
        0xE7C1, 0xE681, 0x2640, 0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041, 0xA001, 0x60C0,
        0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240, 0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480,
        0xA441, 0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41, 0xAA01, 0x6AC0, 0x6B80, 0xAB41,
        0x6900, 0xA9C1, 0xA881, 0x6840, 0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41, 0xBE01,
        0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40, 0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1,
        0xB681, 0x7640, 0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041, 0x5000, 0x90C1, 0x9181,
        0x5140, 0x9301, 0x53C0, 0x5280, 0x9241, 0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
        0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40, 0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901,
        0x59C0, 0x5880, 0x9841, 0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40, 0x4E00, 0x8EC1,
        0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41, 0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680,
        0x8641, 0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
    };

    uint8_t xor8 = 0;
    uint16_t crc16 = 0xFFFF;

    while (len--) {
        xor8 = (*buf++) ^ crc16;
        crc16 >>= 8;
        crc16 ^= table[xor8];
    }

    return crc16;
}

std::vector<uint8_t> getTxBuffer(uint8_t address, uint8_t functionCode, size_t len)
{
    std::vector<uint8_t> buffer;
    buffer.reserve(len + 2);

    buffer.push_back(address);
    buffer.push_back(functionCode);

    return buffer;
}
} // namespace

SerialModbusRTU::SerialModbusRTU(uint8_t uart_nr)
    : m_serial(uart_nr)
    , m_requestTimeout(100)
    , m_requestStartTime(0)
{
}

uint32_t SerialModbusRTU::baudRate() const
{
    return m_serial.baudRate();
}

void SerialModbusRTU::begin(uint32_t baud, uint32_t config, int8_t rxPin, int8_t txPin)
{
    m_serial.onReceive(std::bind(&SerialModbusRTU::onReceiveCb, this), true);
    m_serial.setRxTimeout(3);
    m_serial.setRxBufferSize(MODBUS_ADU);
    m_serial.setTxBufferSize(MODBUS_ADU);
    m_serial.begin(baud, config, rxPin, txPin);
}

void SerialModbusRTU::end()
{
    m_rxReady = false;
    m_requestStartTime = 0;
    m_serial.end();
}

void SerialModbusRTU::setResponseCallback(cbResponse cb)
{
    m_callback = cb;
}

void SerialModbusRTU::setRequestTimeout(std::chrono::milliseconds timeout)
{
    m_requestTimeout = timeout;
}

bool SerialModbusRTU::setPins(int8_t rxPin, int8_t txPin, int8_t ctsPin, int8_t rtsPin)
{
    return m_serial.setPins(rxPin, txPin, ctsPin, rtsPin);
}

bool SerialModbusRTU::isDeviceBusy(uint8_t /*address*/) const
{
    return (m_requestStartTime > 0);
}

void SerialModbusRTU::readCoils(uint8_t address, uint16_t startCoil, uint16_t number)
{
    std::vector<uint8_t> buffer = getTxBuffer(address, FC_READ_COILS, 4);

    buffer.push_back(startCoil >> 8);
    buffer.push_back(startCoil & 0xFF);
    buffer.push_back(number >> 8);
    buffer.push_back(number & 0xFF);

    sendQuery(buffer.data(), buffer.size());
}

void SerialModbusRTU::writeSingleCoil(uint8_t address, uint16_t coil, bool enabled)
{
    std::vector<uint8_t> buffer = getTxBuffer(address, FC_WRITE_COIL, 4);

    buffer.push_back(coil >> 8);
    buffer.push_back(coil & 0xFF);
    buffer.push_back(enabled ? 0xFF : 0x00);
    buffer.push_back(0x00);

    sendQuery(buffer.data(), buffer.size());
}

void SerialModbusRTU::readHoldingRegisters(uint8_t address, uint16_t startReg, uint16_t number)
{
    std::vector<uint8_t> buffer = getTxBuffer(address, FC_READ_REGS, 4);

    buffer.push_back(startReg >> 8);
    buffer.push_back(startReg & 0xFF);
    buffer.push_back(number >> 8);
    buffer.push_back(number & 0xFF);

    sendQuery(buffer.data(), buffer.size());
}

void SerialModbusRTU::writeMultipleRegisters(
    uint8_t address,
    uint16_t startReg,
    uint16_t number,
    const void* data,
    uint8_t len)
{
    std::vector<uint8_t> buffer = getTxBuffer(address, FC_WRITE_REGS, len + 5);

    buffer.push_back(startReg >> 8);
    buffer.push_back(startReg & 0xFF);
    buffer.push_back(number >> 8);
    buffer.push_back(number & 0xFF);
    buffer.push_back(len);
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + len);

    sendQuery(buffer.data(), buffer.size());
}

void SerialModbusRTU::onReceiveCb()
{
    m_rxReady = true;
}

void SerialModbusRTU::sendQuery(const uint8_t* data, size_t len)
{
    if (m_requestStartTime > 0) {
        // Device busy
        if (m_callback)
            m_callback(ec::deviceBusy, data[0], data[1], nullptr, 0);

        return;
    }

    uint16_t crc16 = computeModbusCRC(data, len);

    m_serial.write(data, len);
    m_serial.write(reinterpret_cast<const uint8_t*>(&crc16), sizeof(crc16));

    m_requestStartTime = millis();
}

void SerialModbusRTU::loop()
{
    if (!m_rxReady.exchange(false)) {
        if (m_requestStartTime > 0 && (millis() - m_requestStartTime) > m_requestTimeout.count()) {
            m_requestStartTime = 0;
            if (m_callback)
                m_callback(ec::requestTimeOut, 0, 0, nullptr, 0);
        }

        return;
    }

    m_requestStartTime = 0;

    // Read RX buffer
    int rxLen = m_serial.available();

    std::vector<uint8_t> rxBuffer(rxLen);
    size_t readLen = m_serial.read(rxBuffer.data(), rxBuffer.size());

    if (readLen != rxBuffer.size() || readLen < 5) {
        if (m_callback)
            m_callback(ec::invalidFrameSize, 0, 0, nullptr, 0);
        return;
    }

    // Verify CRC
    uint16_t crc16 = rxBuffer[rxBuffer.size() - 1] << 8;
    crc16 |= rxBuffer[rxBuffer.size() - 2];

    if (crc16 != computeModbusCRC(rxBuffer.data(), rxBuffer.size() - 2)) {
        if (m_callback)
            m_callback(ec::invalidCRC, 0, 0, nullptr, 0);
        return;
    }

    uint8_t address = rxBuffer[0];
    uint8_t functionCode = rxBuffer[1] & 0x7F;

    // Check error response
    if ((rxBuffer[1] & 0x80) == 0x80) {
        if (m_callback)
            m_callback(static_cast<ec>(rxBuffer[2]), address, functionCode, nullptr, 0);
        return;
    }

    if (m_callback)
        m_callback(ec::noError, address, functionCode, rxBuffer.data() + 2, rxBuffer.size() - 4);
}