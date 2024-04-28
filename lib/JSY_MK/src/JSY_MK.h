#pragma once
#include "SerialModbusRTU.h"

// Jian Si Yan Metering module
// www.jsypowermeter.com

class JSY_MK {
public:
    enum class baurate_t : uint8_t {
        _1200 = 3, // 1200bps
        _2400 = 4, // 2400bps
        _4800 = 5, // 4800bps
        _9600 = 6, // 9600bps
        _19200 = 7, // 19200bps
        _38400 = 8 // 38400bps
    };

    typedef std::function<void()> updatedCb;
    typedef std::function<void(SerialModbusRTU::ec)> errorCb;

    JSY_MK(uint8_t uart_nr);
    ~JSY_MK() = default;

    void begin(uint32_t baud, uint32_t config = SERIAL_8N1, int8_t rxPin = -1, int8_t txPin = -1);
    void end();
    void loop();

    bool isBusy() const;

    void setUpdatedCallback(updatedCb cb);
    void setErrorCallback(errorCb cb);

    std::chrono::milliseconds getUpdatePeriod() const;
    void setUpdatePeriod(std::chrono::milliseconds period);

    void setBaudrate(baurate_t baudrate);

    uint8_t getAddress() const;
    size_t getChannelNumber() const;

    size_t getModel() const;
    std::string getModelAsString() const;
    std::string getManufacturer() const;

    size_t getVersion() const;
    std::string getVersionAsString() const;

    size_t getVoltageRange() const;
    size_t getCurrentRange() const;

    bool isNegative() const;
    float getVoltage() const;
    float getCurrent() const;
    float getPower() const;
    float getPowerFactor() const;
    float getFrequency() const;

    float getPositiveEnergy() const;
    float getNegativeEnergy() const;

    void resetEnergy();

private:
    void responseHandler(
        SerialModbusRTU::ec exceptionCode,
        uint8_t deviceAddress,
        uint8_t functionCode,
        const void* data,
        size_t len);

private:
    SerialModbusRTU m_serialRTU;
    std::chrono::milliseconds m_updatePeriod;
    time_t m_lastUpdateTime;
    updatedCb m_updateCallback;
    errorCb m_errorCallback;

    uint16_t m_model {};
    uint16_t m_version {};
    size_t m_voltageRange {};
    size_t m_CurrentRange {};

    bool m_isNegative {};
    float m_voltage {};
    float m_current {};
    float m_power {};
    float m_powerFactor {};
    float m_frequency {};
    float m_positiveEnergy {};
    float m_negativeEnergy {};
};
