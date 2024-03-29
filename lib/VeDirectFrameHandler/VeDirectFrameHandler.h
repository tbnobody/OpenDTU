/* frameHandler.h
 *
 * Arduino library to read from Victron devices using VE.Direct protocol.
 * Derived from Victron framehandler reference implementation.
 *
 * 2020.05.05 - 0.2 - initial release
 * 2021.02.23 - 0.3 - change frameLen to 22 per VE.Direct Protocol version 3.30
 * 2022.08.20 - 0.4 - changes for OpenDTU
 *
 */

#pragma once

#include <Arduino.h>
#include <array>
#include <memory>
#include <utility>
#include <deque>
#include "VeDirectData.h"

template<typename T>
class VeDirectFrameHandler {
public:
    void loop();                                 // main loop to read ve.direct data
    uint32_t getLastUpdate() const;              // timestamp of last successful frame read
    bool isDataValid() const;                    // return true if data valid and not outdated

protected:
    VeDirectFrameHandler();
    void init(char const* who, int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging, uint16_t hwSerialPort);

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
    virtual void frameValidEvent() = 0;
    int hexRxEvent(uint8_t);

    std::unique_ptr<HardwareSerial> _vedirectSerial;
    int _state;                                // current state
    int _prevState;                            // previous state
    uint8_t _checksum;                         // checksum value
    char * _textPointer;                       // pointer to the private buffer we're writing to, name or value
    int _hexSize;                               // length of hex buffer
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
