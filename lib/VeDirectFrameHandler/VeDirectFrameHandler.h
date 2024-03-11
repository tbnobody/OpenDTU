/* frameHandler.h
 *
 * Arduino library to read from Victron devices using VE.Direct protocol.
 * Derived from Victron framehandler reference implementation.
 *
 * 2020.05.05 - 0.2 - initial release
 * 2021.02.23 - 0.3 - change frameLen to 22 per VE.Direct Protocol version 3.30
 * 2022.08.20 - 0.4 - changes for OpenDTU
 * 2024.03.08 - 0.4 - adds the ability to send hex commands and disassemble hex messages
 *
 */

#pragma once

#include <Arduino.h>
#include <array>
#include <memory>
#include <utility>
#include <deque>
#include "VeDirectData.h"

// hex send commands
enum VeDirectHexCommand {
    ENTER_BOOT = 0x00,
    PING = 0x01,
    APP_VERSION = 0x02,
    PRODUCT_ID = 0x04,
    RESTART = 0x06,
    GET = 0x07,
    SET = 0x08,
    ASYNC = 0x0A,
    UNKNOWN = 0x0F
};

// hex receive responses
enum VeDirectHexResponse {
    R_DONE = 0x01,
    R_UNKNOWN = 0x03,
    R_ERROR = 0x04,
    R_PING = 0x05,
    R_GET = 0x07,
    R_SET = 0x08,
    R_ASYNC = 0x0A,
};

// hex response data, contains the disassembeled hex message, forwarded to virtual funktion hexDataHandler()
struct VeDirectHexData {
    VeDirectHexResponse rsp;        // hex response type   
    uint16_t id;                    // register address
    uint8_t flag;                   // flag
    uint32_t value;                 // value from register
    char text[VE_MAX_HEX_LEN];      // text/string response
};

template<typename T>
class VeDirectFrameHandler {
public:
    virtual void loop();                         // main loop to read ve.direct data
    uint32_t getLastUpdate() const;              // timestamp of last successful frame read
    bool isDataValid() const;                    // return true if data valid and not outdated
    T const& getData() const { return _tmpFrame; }
    bool sendHexCommand(VeDirectHexCommand cmd, uint16_t id = 0, uint32_t value = 0, uint8_t valunibble = 0);   // send hex commands via ve.direct

protected:
    VeDirectFrameHandler();
    void init(char const* who, int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging, uint16_t hwSerialPort);
    virtual void hexDataHandler(VeDirectHexData const &data) { } // handles the disassembeled hex response

    bool _verboseLogging;
    Print* _msgOut;
    uint32_t _lastUpdate;

    T _tmpFrame;

private:
    void reset();
    void dumpDebugBuffer();
    void rxData(uint8_t inbyte);              // byte of serial data
    void processTextData(std::string const& name, std::string const& value);
    virtual bool processTextDataDerived(std::string const& name, std::string const& value) = 0;
    virtual void frameValidEvent() { }
    int hexRxEvent(uint8_t);
    bool disassembleHexData(VeDirectHexData &data);     //return true if disassembling was possible

    std::unique_ptr<HardwareSerial> _vedirectSerial;
    int _state;                                // current state
    int _prevState;                            // previous state
    uint8_t _checksum;                         // checksum value
    char * _textPointer;                       // pointer to the private buffer we're writing to, name or value
    int _hexSize;                               // length of hex buffer
    char _hexBuffer[VE_MAX_HEX_LEN] = { };	   // buffer for received hex frames
    char _name[VE_MAX_VALUE_LEN];              // buffer for the field name
    char _value[VE_MAX_VALUE_LEN];             // buffer for the field value
    std::array<uint8_t, 512> _debugBuffer;
    unsigned _debugIn;
    uint32_t _lastByteMillis;
    char _logId[32];

    /**
     * not every frame contains every value the device is communicating, i.e.,
     * a set of values can be fragmented across multiple frames. frames can be
     * invalid. in order to only process data from valid frames, we add data
     * to this queue and only process it once the frame was found to be valid.
     * this also handles fragmentation nicely, since there is no need to reset
     * our data buffer. we simply update the interpreted data from this event
     * queue, which is fine as we know the source frame was valid.
     */
    std::deque<std::pair<std::string, std::string>> _textData;
};

template class VeDirectFrameHandler<veMpptStruct>;
template class VeDirectFrameHandler<veShuntStruct>;
