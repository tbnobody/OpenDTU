#pragma once
#include <atomic>
#include <chrono>
#include <functional>

#include <HardwareSerial.h>

class SerialModbusRTU
{
public:
	enum class ec : uint8_t
	{
		noError = 0x00,
		// MODBUS errors
		illegalFunction = 0x01,
		illegalDataAddress = 0x02,
		illegalDataValue = 0x03,
		serverDeviceFailure = 0x04,
		acknowledge = 0x05,
		serverDeviceBusy = 0x06,
		memoryParityError = 0x08,
		gatewayPathUnavailable = 0x0A,
		gatewayTargetDeviceFailed = 0x0B,
		// Class errors
		invalidFrameSize = 0x20,
		invalidCRC = 0x21,
		requestTimeOut = 0x22,
		deviceBusy = 0x23
	};

	typedef std::function<
		void(ec exceptionCode, uint8_t deviceAddress, uint8_t functionCode, const void* data, size_t len)>
		cbResponse;

	SerialModbusRTU(uint8_t uart_nr);
	~SerialModbusRTU() = default;

	uint32_t baudRate() const;

	void begin(uint32_t baud, uint32_t config = SERIAL_8N1, int8_t rxPin = -1, int8_t txPin = -1);
	void end();
	void loop();

	// setResponseCallback and setRequestTimeout shall be called before begin()
	void setResponseCallback(cbResponse cb);
	void setRequestTimeout(std::chrono::milliseconds timeout);

	// SetPins shall be called after begin()
	bool setPins(int8_t rxPin, int8_t txPin, int8_t ctsPin = -1, int8_t rtsPin = -1);

	[[nodiscard]] bool isDeviceBusy(uint8_t address) const;

	void readCoils(uint8_t address, uint16_t startCoil, uint16_t number);
	void writeSingleCoil(uint8_t address, uint16_t coil, bool enabled);

	void readHoldingRegisters(uint8_t address, uint16_t startReg, uint16_t number);
	void writeMultipleRegisters(uint8_t address, uint16_t startReg, uint16_t number, const void* data, uint8_t len);

private:
	void onReceiveCb();
	void sendQuery(const uint8_t* data, size_t len);

private:
	mutable HardwareSerial m_serial;
	std::atomic<bool> m_rxReady;
	cbResponse m_callback;
	std::chrono::milliseconds m_requestTimeout;
	time_t m_requestStartTime;
};
