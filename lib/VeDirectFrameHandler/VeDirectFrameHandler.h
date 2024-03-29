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
#include <frozen/string.h>
#include <frozen/map.h>
#include <memory>

#define VE_MAX_VALUE_LEN 33 // VE.Direct Protocol: max value size is 33 including /0
#define VE_MAX_HEX_LEN 100 // Maximum size of hex frame - max payload 34 byte (=68 char) + safe buffer

class VeDirectFrameHandler {
public:
    void loop();                                 // main loop to read ve.direct data
    uint32_t getLastUpdate() const;              // timestamp of last successful frame read

protected:
    VeDirectFrameHandler();
    void init(char const* who, int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging, uint16_t hwSerialPort);

    bool _verboseLogging;
    Print* _msgOut;
    uint32_t _lastUpdate;

    typedef struct {
        uint16_t PID = 0;               // product id
        char SER[VE_MAX_VALUE_LEN];     // serial number
        char FW[VE_MAX_VALUE_LEN];      // firmware release number
        double V = 0;                   // battery voltage in V
        double I = 0;                   // battery current in A
        double E = 0;                   // efficiency in percent (calculated, moving average)

        frozen::string const& getPidAsString() const; // product ID as string
    } veStruct;

    bool textRxEvent(char* name, char* value, veStruct& frame);
    bool isDataValid(veStruct const& frame) const;      // return true if data valid and not outdated

    template<typename T, size_t L>
    static frozen::string const& getAsString(frozen::map<T, frozen::string, L> const& values, T val)
    {
        auto pos = values.find(val);
        if (pos == values.end()) {
            static constexpr frozen::string dummy("???");
            return dummy;
        }
        return pos->second;
    }

private:
    void setLastUpdate();                     // set timestampt after successful frame read
    void dumpDebugBuffer();
    void rxData(uint8_t inbyte);              // byte of serial data
    virtual void textRxEvent(char *, char *) = 0;
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
};
