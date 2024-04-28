#include "JSY_MK.h"
#include <byteswap.h>

namespace {
constexpr uint8_t DEVICE_ADDRESS = 1;
constexpr std::chrono::seconds DEFAULT_UPDATE_PERIOD(1);

struct parameterRaw {
    uint8_t len;
    uint16_t model;
    uint16_t version;
    uint16_t voltageRange;
    uint16_t CurrentRange;
} __attribute__((__packed__));

struct measurementRaw {
    uint8_t len;
    uint16_t voltage;
    uint16_t current;
    uint16_t power;
    uint32_t positiveEnergy;
    uint16_t powerFactor;
    uint32_t negativeEnergy;
    uint16_t powerDirect;
    uint16_t frequency;
} __attribute__((__packed__));
} // namespace

JSY_MK::JSY_MK(uint8_t uart_nr)
    : m_serialRTU(uart_nr)
    , m_updatePeriod(DEFAULT_UPDATE_PERIOD)
    , m_lastUpdateTime(0)
{
}

void JSY_MK::begin(uint32_t baud, uint32_t config, int8_t rxPin, int8_t txPin)
{
    m_serialRTU.setRequestTimeout(std::chrono::milliseconds(500));
    m_serialRTU.setResponseCallback(std::bind(
        &JSY_MK::responseHandler,
        this,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3,
        std::placeholders::_4,
        std::placeholders::_5));

    m_serialRTU.begin(baud, config, rxPin, txPin);
    m_serialRTU.readHoldingRegisters(DEVICE_ADDRESS, 0x0000, 4);
}

void JSY_MK::end()
{
    m_serialRTU.end();
    m_model = 0;
}

void JSY_MK::loop()
{
    m_serialRTU.loop();

    if (m_updatePeriod > std::chrono::milliseconds::zero() && !m_serialRTU.isDeviceBusy(DEVICE_ADDRESS) && m_model != 0 && (m_lastUpdateTime == 0 || (millis() - m_lastUpdateTime) > m_updatePeriod.count())) {
        m_serialRTU.readHoldingRegisters(DEVICE_ADDRESS, 0x0048, 10);
        m_lastUpdateTime = millis();
    }
}

bool JSY_MK::isBusy() const
{
    return m_serialRTU.isDeviceBusy(DEVICE_ADDRESS);
}

void JSY_MK::setUpdatedCallback(updatedCb cb)
{
    m_updateCallback = cb;
}

void JSY_MK::setErrorCallback(errorCb cb)
{
    m_errorCallback = cb;
}

void JSY_MK::responseHandler(
    SerialModbusRTU::ec exceptionCode,
    uint8_t deviceAddress,
    uint8_t functionCode,
    const void* data,
    size_t len)
{
    if (exceptionCode != SerialModbusRTU::ec::noError) {
        if (m_model == 0) {
            // Retry init
            m_serialRTU.readHoldingRegisters(DEVICE_ADDRESS, 0x0000, 4);
        }

        if (m_errorCallback)
            m_errorCallback(exceptionCode);

        return;
    }

    if (deviceAddress != DEVICE_ADDRESS)
        return; // Ignore

    if (data == nullptr)
        return; // Error

    if (len == sizeof(parameterRaw)) {
        const auto* buffer = static_cast<const parameterRaw*>(data);
        if (buffer->len != sizeof(parameterRaw) - 1)
            return; // Error

        m_model = __bswap_16(buffer->model);
        m_version = __bswap_16(buffer->version);
        m_voltageRange = __bswap_16(buffer->voltageRange);
        m_CurrentRange = __bswap_16(buffer->CurrentRange) / 10;

        if (m_version == 0)
            m_version = 0x0100;
    } else if (len == sizeof(measurementRaw)) {
        const auto* buffer = static_cast<const measurementRaw*>(data);
        if (buffer->len != sizeof(measurementRaw) - 1)
            return; // Error

        m_voltage = static_cast<float>(__bswap_16(buffer->voltage)) / 100.F;
        m_current = static_cast<float>(__bswap_16(buffer->current)) / 100.F;
        m_power = static_cast<float>(__bswap_16(buffer->power));
        m_powerFactor = static_cast<float>(__bswap_16(buffer->powerFactor)) / 1000.F;
        m_frequency = static_cast<float>(__bswap_16(buffer->frequency)) / 100.F;

        m_isNegative = (buffer->powerDirect != 0);
        m_positiveEnergy = static_cast<float>(__bswap_32(buffer->positiveEnergy)) / 3200.F;
        m_negativeEnergy = static_cast<float>(__bswap_32(buffer->negativeEnergy)) / 3200.F;

        if (m_updateCallback)
            m_updateCallback();
    } else if (len == 2) {
    }
}

std::chrono::milliseconds JSY_MK::getUpdatePeriod() const
{
    return m_updatePeriod;
}

void JSY_MK::setUpdatePeriod(std::chrono::milliseconds period)
{
    m_updatePeriod = period;
}

void JSY_MK::setBaudrate(baurate_t baudrate)
{
    if (m_serialRTU.isDeviceBusy(DEVICE_ADDRESS))
        return;

    std::array<uint8_t, 2> buffer = { DEVICE_ADDRESS, static_cast<uint8_t>(baudrate) };
    m_serialRTU.writeMultipleRegisters(DEVICE_ADDRESS, 0x0004, 1, buffer.data(), buffer.size());
}

uint8_t JSY_MK::getAddress() const
{
    return DEVICE_ADDRESS;
}

std::string JSY_MK::getManufacturer() const
{
    return "Shenzhen JianSiYan Technologies";
}

size_t JSY_MK::getChannelNumber() const
{
    return 1;
}

size_t JSY_MK::getModel() const
{
    return m_model;
}

std::string JSY_MK::getModelAsString() const
{
    std::array<char, 20> buffer = {};
    int len = std::snprintf(buffer.data(), buffer.size(), "JSY-MK-%XT", m_model);
    return len > 0 ? std::string(buffer.data(), len) : std::string();
}

size_t JSY_MK::getVersion() const
{
    return m_version;
}

std::string JSY_MK::getVersionAsString() const
{
    std::array<char, 10> buffer = {};
    int len = std::snprintf(buffer.data(), buffer.size(), "%X.%X", (m_version >> 8) & 0x0F, (m_version >> 4) & 0x0F);
    return len > 0 ? std::string(buffer.data(), len) : std::string();
}

size_t JSY_MK::getVoltageRange() const
{
    return m_voltageRange;
}

size_t JSY_MK::getCurrentRange() const
{
    return m_CurrentRange;
}

bool JSY_MK::isNegative() const
{
    return m_isNegative;
}

float JSY_MK::getVoltage() const
{
    return m_voltage;
}

float JSY_MK::getCurrent() const
{
    return m_current;
}

float JSY_MK::getPower() const
{
    return m_power;
}

float JSY_MK::getPowerFactor() const
{
    return m_powerFactor;
}

float JSY_MK::getFrequency() const
{
    return m_frequency;
}

float JSY_MK::getPositiveEnergy() const
{
    return m_positiveEnergy;
}

float JSY_MK::getNegativeEnergy() const
{
    return m_negativeEnergy;
}

void JSY_MK::resetEnergy()
{
    if (m_serialRTU.isDeviceBusy(DEVICE_ADDRESS))
        return;

    // Registers 0x000C, 0x000D
    // Data 00000000
    std::array<uint8_t, 4> buffer = {};
    m_serialRTU.writeMultipleRegisters(DEVICE_ADDRESS, 0x000C, 2, buffer.data(), buffer.size());
}
