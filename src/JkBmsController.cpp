#include <Arduino.h>
#include "Configuration.h"
#include "HardwareSerial.h"
#include "PinMapping.h"
#include "MessageOutput.h"
#include "JkBmsDataPoints.h"
#include "JkBmsController.h"
#include <map>

//#define JKBMS_DUMMY_SERIAL

#ifdef JKBMS_DUMMY_SERIAL
class DummySerial {
    public:
        DummySerial() = default;
        void begin(uint32_t, uint32_t, int8_t, int8_t) {
            MessageOutput.println("JK BMS Dummy Serial: begin()");
        }
        void end() { MessageOutput.println("JK BMS Dummy Serial: end()"); }
        void flush() { }
        bool availableForWrite() const { return true; }
        size_t write(const uint8_t *buffer, size_t size) {
            MessageOutput.printf("JK BMS Dummy Serial: write(%d Bytes)\r\n", size);
            _byte_idx = 0;
            _msg_idx = (_msg_idx + 1) % _data.size();
            return size;
        }
        bool available() const {
            return _byte_idx < _data[_msg_idx].size();
        }
        int read() {
            if (_byte_idx >= _data[_msg_idx].size()) { return 0; }
            return _data[_msg_idx][_byte_idx++];
        }

    private:
        std::vector<std::vector<uint8_t>> const _data =
        {
            {
                0x4e, 0x57, 0x01, 0x21, 0x00, 0x00, 0x00, 0x00,
                0x06, 0x00, 0x01, 0x79, 0x30, 0x01, 0x0c, 0xfb,
                0x02, 0x0c, 0xfb, 0x03, 0x0c, 0xfb, 0x04, 0x0c,
                0xfb, 0x05, 0x0c, 0xfb, 0x06, 0x0c, 0xfb, 0x07,
                0x0c, 0xfb, 0x08, 0x0c, 0xf7, 0x09, 0x0d, 0x01,
                0x0a, 0x0c, 0xf9, 0x0b, 0x0c, 0xfb, 0x0c, 0x0c,
                0xfb, 0x0d, 0x0c, 0xfb, 0x0e, 0x0c, 0xf8, 0x0f,
                0x0c, 0xf9, 0x10, 0x0c, 0xfb, 0x80, 0x00, 0x1a,
                0x81, 0x00, 0x12, 0x82, 0x00, 0x12, 0x83, 0x14,
                0xc3, 0x84, 0x83, 0xf4, 0x85, 0x2e, 0x86, 0x02,
                0x87, 0x00, 0x15, 0x89, 0x00, 0x00, 0x13, 0x52,
                0x8a, 0x00, 0x10, 0x8b, 0x00, 0x00, 0x8c, 0x00,
                0x03, 0x8e, 0x16, 0x80, 0x8f, 0x12, 0xc0, 0x90,
                0x0e, 0x10, 0x91, 0x0c, 0xda, 0x92, 0x00, 0x05,
                0x93, 0x0b, 0xb8, 0x94, 0x0c, 0x80, 0x95, 0x00,
                0x05, 0x96, 0x01, 0x2c, 0x97, 0x00, 0x28, 0x98,
                0x01, 0x2c, 0x99, 0x00, 0x28, 0x9a, 0x00, 0x1e,
                0x9b, 0x0b, 0xb8, 0x9c, 0x00, 0x0a, 0x9d, 0x01,
                0x9e, 0x00, 0x64, 0x9f, 0x00, 0x50, 0xa0, 0x00,
                0x64, 0xa1, 0x00, 0x64, 0xa2, 0x00, 0x14, 0xa3,
                0x00, 0x46, 0xa4, 0x00, 0x46, 0xa5, 0x00, 0x00,
                0xa6, 0x00, 0x02, 0xa7, 0xff, 0xec, 0xa8, 0xff,
                0xf6, 0xa9, 0x10, 0xaa, 0x00, 0x00, 0x00, 0xe6,
                0xab, 0x01, 0xac, 0x01, 0xad, 0x04, 0x4d, 0xae,
                0x01, 0xaf, 0x00, 0xb0, 0x00, 0x0a, 0xb1, 0x14,
                0xb2, 0x32, 0x32, 0x31, 0x31, 0x38, 0x37, 0x00,
                0x00, 0x00, 0x00, 0xb3, 0x00, 0xb4, 0x62, 0x65,
                0x6b, 0x69, 0x00, 0x00, 0x00, 0x00, 0xb5, 0x32,
                0x33, 0x30, 0x36, 0xb6, 0x00, 0x01, 0x4a, 0xc3,
                0xb7, 0x31, 0x31, 0x2e, 0x58, 0x57, 0x5f, 0x53,
                0x31, 0x31, 0x2e, 0x32, 0x36, 0x32, 0x48, 0x5f,
                0xb8, 0x00, 0xb9, 0x00, 0x00, 0x00, 0xe6, 0xba,
                0x62, 0x65, 0x6b, 0x69, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x4a, 0x4b, 0x5f, 0x42,
                0x31, 0x41, 0x32, 0x34, 0x53, 0x31, 0x35, 0x50,
                0xc0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00,
                0x00, 0x53, 0xbb
            },
            {
                0x4e, 0x57, 0x01, 0x21, 0x00, 0x00, 0x00, 0x00,
                0x06, 0x00, 0x01, 0x79, 0x30, 0x01, 0x0c, 0xc0,
                0x02, 0x0c, 0xc1, 0x03, 0x0c, 0xc0, 0x04, 0x0c,
                0xc4, 0x05, 0x0c, 0xc4, 0x06, 0x0c, 0xc2, 0x07,
                0x0c, 0xc2, 0x08, 0x0c, 0xc1, 0x09, 0x0c, 0xba,
                0x0a, 0x0c, 0xc1, 0x0b, 0x0c, 0xc2, 0x0c, 0x0c,
                0xc2, 0x0d, 0x0c, 0xc2, 0x0e, 0x0c, 0xc4, 0x0f,
                0x0c, 0xc2, 0x10, 0x0c, 0xc1, 0x80, 0x00, 0x1b,
                0x81, 0x00, 0x1b, 0x82, 0x00, 0x1a, 0x83, 0x14,
                0x68, 0x84, 0x03, 0x70, 0x85, 0x3c, 0x86, 0x02,
                0x87, 0x00, 0x19, 0x89, 0x00, 0x00, 0x16, 0x86,
                0x8a, 0x00, 0x10, 0x8b, 0x00, 0x00, 0x8c, 0x00,
                0x07, 0x8e, 0x16, 0x80, 0x8f, 0x12, 0xc0, 0x90,
                0x0e, 0x10, 0x91, 0x0c, 0xda, 0x92, 0x00, 0x05,
                0x93, 0x0b, 0xb8, 0x94, 0x0c, 0x80, 0x95, 0x00,
                0x05, 0x96, 0x01, 0x2c, 0x97, 0x00, 0x28, 0x98,
                0x01, 0x2c, 0x99, 0x00, 0x28, 0x9a, 0x00, 0x1e,
                0x9b, 0x0b, 0xb8, 0x9c, 0x00, 0x0a, 0x9d, 0x01,
                0x9e, 0x00, 0x64, 0x9f, 0x00, 0x50, 0xa0, 0x00,
                0x64, 0xa1, 0x00, 0x64, 0xa2, 0x00, 0x14, 0xa3,
                0x00, 0x46, 0xa4, 0x00, 0x46, 0xa5, 0x00, 0x00,
                0xa6, 0x00, 0x02, 0xa7, 0xff, 0xec, 0xa8, 0xff,
                0xf6, 0xa9, 0x10, 0xaa, 0x00, 0x00, 0x00, 0xe6,
                0xab, 0x01, 0xac, 0x01, 0xad, 0x04, 0x4d, 0xae,
                0x01, 0xaf, 0x00, 0xb0, 0x00, 0x0a, 0xb1, 0x14,
                0xb2, 0x32, 0x32, 0x31, 0x31, 0x38, 0x37, 0x00,
                0x00, 0x00, 0x00, 0xb3, 0x00, 0xb4, 0x62, 0x65,
                0x6b, 0x69, 0x00, 0x00, 0x00, 0x00, 0xb5, 0x32,
                0x33, 0x30, 0x36, 0xb6, 0x00, 0x01, 0x7f, 0x2a,
                0xb7, 0x31, 0x31, 0x2e, 0x58, 0x57, 0x5f, 0x53,
                0x31, 0x31, 0x2e, 0x32, 0x36, 0x32, 0x48, 0x5f,
                0xb8, 0x00, 0xb9, 0x00, 0x00, 0x00, 0xe6, 0xba,
                0x62, 0x65, 0x6b, 0x69, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x4a, 0x4b, 0x5f, 0x42,
                0x31, 0x41, 0x32, 0x34, 0x53, 0x31, 0x35, 0x50,
                0xc0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00,
                0x00, 0x4f, 0xc1
            }
        };
        size_t _msg_idx = 0;
        size_t _byte_idx = 0;
};
DummySerial HwSerial;
#else
HardwareSerial HwSerial(2);
#endif

namespace JkBms {

bool Controller::init(bool verboseLogging)
{
    _verboseLogging = verboseLogging;

    std::string ifcType = "transceiver";
    if (Interface::Transceiver != getInterface()) { ifcType = "TTL-UART"; }
    MessageOutput.printf("[JK BMS] Initialize %s interface...\r\n", ifcType.c_str());

    const PinMapping_t& pin = PinMapping.get();
    MessageOutput.printf("[JK BMS] rx = %d, rxen = %d, tx = %d, txen = %d\r\n",
            pin.battery_rx, pin.battery_rxen, pin.battery_tx, pin.battery_txen);

    if (pin.battery_rx < 0 || pin.battery_tx < 0) {
        MessageOutput.println(F("[JK BMS] Invalid RX/TX pin config"));
        return false;
    }

    HwSerial.begin(115200, SERIAL_8N1, pin.battery_rx, pin.battery_tx);
    HwSerial.flush();

    if (Interface::Transceiver != getInterface()) { return true; }

    _rxEnablePin = pin.battery_rxen;
    _txEnablePin = pin.battery_txen;

    if (_rxEnablePin < 0 || _txEnablePin < 0) {
        MessageOutput.println(F("[JK BMS] Invalid transceiver pin config"));
        return false;
    }

    pinMode(_rxEnablePin, OUTPUT);
    pinMode(_txEnablePin, OUTPUT);

    return true;
}

void Controller::deinit()
{
    HwSerial.end();

    if (_rxEnablePin > 0) { pinMode(_rxEnablePin, INPUT); }
    if (_txEnablePin > 0) { pinMode(_txEnablePin, INPUT); }
}

Controller::Interface Controller::getInterface() const
{
    CONFIG_T& config = Configuration.get();
    if (0x00 == config.Battery_JkBmsInterface) { return Interface::Uart; }
    if (0x01 == config.Battery_JkBmsInterface) { return Interface::Transceiver; }
    return Interface::Invalid;
}

std::string const& Controller::getStatusText(Controller::Status status)
{
    static const std::string missing =  "programmer error: missing status text";

    static const std::map<Status, const std::string> texts = {
        { Status::Timeout, "timeout wating for response from BMS" },
        { Status::WaitingForPollInterval, "waiting for poll interval to elapse" },
        { Status::HwSerialNotAvailableForWrite, "UART is not available for writing" },
        { Status::BusyReading, "busy waiting for or reading a message from the BMS" },
        { Status::RequestSent, "request for data sent" },
        { Status::FrameCompleted, "a whole frame was received" }
    };

    auto iter = texts.find(status);
    if (iter == texts.end()) { return missing; }

    return iter->second;
}

void Controller::announceStatus(Controller::Status status)
{
    if (_lastStatus == status && millis() < _lastStatusPrinted + 10 * 1000) { return; }

    MessageOutput.printf("[%11.3f] JK BMS: %s\r\n",
        static_cast<double>(millis())/1000, getStatusText(status).c_str());

    _lastStatus = status;
    _lastStatusPrinted = millis();
}

void Controller::sendRequest(uint8_t pollInterval)
{
    if (ReadState::Idle != _readState) {
        return announceStatus(Status::BusyReading);
    }

    if ((millis() - _lastRequest) < pollInterval * 1000) {
        return announceStatus(Status::WaitingForPollInterval);
    }

    if (!HwSerial.availableForWrite()) {
        return announceStatus(Status::HwSerialNotAvailableForWrite);
    }

    SerialCommand readAll(SerialCommand::Command::ReadAll);

    if (Interface::Transceiver == getInterface()) {
        digitalWrite(_rxEnablePin, HIGH); // disable reception (of our own data)
        digitalWrite(_txEnablePin, HIGH); // enable transmission
    }

    HwSerial.write(readAll.data(), readAll.size());

    if (Interface::Transceiver == getInterface()) {
        HwSerial.flush();
        digitalWrite(_rxEnablePin, LOW); // enable reception
        digitalWrite(_txEnablePin, LOW); // disable transmission (free the bus)
    }

    _lastRequest = millis();

    setReadState(ReadState::WaitingForFrameStart);
    return announceStatus(Status::RequestSent);
}

void Controller::loop()
{
    CONFIG_T& config = Configuration.get();
    uint8_t pollInterval = config.Battery_JkBmsPollingInterval;

    while (HwSerial.available()) {
        rxData(HwSerial.read());
    }

    sendRequest(pollInterval);

    if (millis() > _lastRequest + 2 * pollInterval * 1000 + 250) {
        reset();
        return announceStatus(Status::Timeout);
    }
}

void Controller::rxData(uint8_t inbyte)
{
    _buffer.push_back(inbyte);

    switch(_readState) {
        case ReadState::Idle: // unsolicited message from BMS
        case ReadState::WaitingForFrameStart:
            if (inbyte == 0x4E) {
                return setReadState(ReadState::FrameStartReceived);
            }
            break;
        case ReadState::FrameStartReceived:
            if (inbyte == 0x57) {
                return setReadState(ReadState::StartMarkerReceived);
            }
            break;
        case ReadState::StartMarkerReceived:
            _frameLength = inbyte << 8 | 0x00;
            return setReadState(ReadState::FrameLengthMsbReceived);
            break;
        case ReadState::FrameLengthMsbReceived:
            _frameLength |= inbyte;
            _frameLength -= 2; // length field already read
            return setReadState(ReadState::ReadingFrame);
            break;
        case ReadState::ReadingFrame:
            _frameLength--;
            if (_frameLength == 0) {
                return frameComplete();
            }
            return setReadState(ReadState::ReadingFrame);
            break;
    }

    reset();
}

void Controller::reset()
{
    _buffer.clear();
    return setReadState(ReadState::Idle);
}

void Controller::frameComplete()
{
    announceStatus(Status::FrameCompleted);

    if (_verboseLogging) {
        double ts = static_cast<double>(millis())/1000;
        MessageOutput.printf("[%11.3f] JK BMS: raw data (%d Bytes):",
            ts, _buffer.size());
        for (size_t ctr = 0; ctr < _buffer.size(); ++ctr) {
            if (ctr % 16 == 0) {
                MessageOutput.printf("\r\n[%11.3f] JK BMS: ", ts);
            }
            MessageOutput.printf("%02x ", _buffer[ctr]);
        }
        MessageOutput.println();
    }

    auto pResponse = std::make_unique<SerialResponse>(std::move(_buffer), _protocolVersion);
    if (pResponse->isValid()) {
        processDataPoints(pResponse->getDataPoints());
    } // if invalid, error message has been produced by SerialResponse c'tor

    reset();
}

void Controller::processDataPoints(DataPointContainer const& dataPoints)
{
    _stats->updateFrom(dataPoints);

    using Label = JkBms::DataPointLabel;

    auto oProtocolVersion = dataPoints.get<Label::ProtocolVersion>();
    if (oProtocolVersion.has_value()) { _protocolVersion = *oProtocolVersion; }

    if (!_verboseLogging) { return; }

    auto iter = dataPoints.cbegin();
    while ( iter != dataPoints.cend() ) {
        MessageOutput.printf("[%11.3f] JK BMS: %s: %s%s\r\n",
            static_cast<double>(iter->second.getTimestamp())/1000,
            iter->second.getLabelText().c_str(),
            iter->second.getValueText().c_str(),
            iter->second.getUnitText().c_str());
        ++iter;
    }
}

} /* namespace JkBms */
