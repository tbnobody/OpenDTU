#pragma once

#include <memory>
#include <vector>
#include <frozen/string.h>

#include "Battery.h"
#include "JkBmsSerialMessage.h"

class DataPointContainer;

namespace JkBms {

uint8_t constexpr HwSerialPort = ((ARDUINO_USB_CDC_ON_BOOT != 1)?2:0);

class Controller : public BatteryProvider {
    public:
        Controller() = default;

        bool init(bool verboseLogging) final;
        void deinit() final;
        void loop() final;
        std::shared_ptr<BatteryStats> getStats() const final { return _stats; }
        int usedHwUart() const final { return HwSerialPort; }

    private:
        enum class Status : unsigned {
            Initializing,
            Timeout,
            WaitingForPollInterval,
            HwSerialNotAvailableForWrite,
            BusyReading,
            RequestSent,
            FrameCompleted
        };

        frozen::string const& getStatusText(Status status);
        void announceStatus(Status status);
        void sendRequest(uint8_t pollInterval);
        void rxData(uint8_t inbyte);
        void reset();
        void frameComplete();
        void processDataPoints(DataPointContainer const& dataPoints);

        enum class Interface : unsigned {
            Invalid,
            Uart,
            Transceiver
        };

        Interface getInterface() const;

        enum class ReadState : unsigned {
            Idle,
            WaitingForFrameStart,
            FrameStartReceived,
            StartMarkerReceived,
            FrameLengthMsbReceived,
            ReadingFrame
        };
        ReadState _readState;
        void setReadState(ReadState state) {
            _readState = state;
        }

        bool _verboseLogging = true;
        int8_t _rxEnablePin = -1;
        int8_t _txEnablePin = -1;
        Status _lastStatus = Status::Initializing;
        uint32_t _lastStatusPrinted = 0;
        uint32_t _lastRequest = 0;
        uint16_t _frameLength = 0;
        uint8_t _protocolVersion = -1;
        SerialResponse::tData _buffer = {};
        std::shared_ptr<JkBmsBatteryStats> _stats =
            std::make_shared<JkBmsBatteryStats>();
};

} /* namespace JkBms */
