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

#define VE_MAX_VALUE_LEN 33 // VE.Direct Protocol: max value size is 33 including /0
#define VE_MAX_HEX_LEN 100 // Maximum size of hex frame - max payload 34 byte (=68 char) + safe buffer

typedef struct {
    uint16_t PID = 0;               // product id
    char SER[VE_MAX_VALUE_LEN];     // serial number
    char FW[VE_MAX_VALUE_LEN];      // firmware release number
    int32_t P = 0;                  // battery output power in W (calculated)
    double V = 0;                   // battery voltage in V
    double I = 0;                   // battery current in A
    double E = 0;                   // efficiency in percent (calculated, moving average)
} veStruct;

class VeDirectFrameHandler {
public:
    VeDirectFrameHandler();
    void setVerboseLogging(bool verboseLogging);
    virtual void init(int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging, uint16_t hwSerialPort);
    void loop();                                 // main loop to read ve.direct data
    unsigned long getLastUpdate();               // timestamp of last successful frame read
    bool isDataValid(veStruct frame);                          // return true if data valid and not outdated
    String getPidAsString(uint16_t pid);      // product id as string
    String getErrAsString(uint8_t err);      // errer state as string

protected:
    void textRxEvent(char *, char *, veStruct& );

    bool _verboseLogging;
    Print* _msgOut;
    uint32_t _lastUpdate;

private:
    void setLastUpdate();                     // set timestampt after successful frame read
    void dumpDebugBuffer();
    void rxData(uint8_t inbyte);              // byte of serial data
    virtual void textRxEvent(char *, char *) = 0;
    virtual void frameEndEvent(bool) = 0;                 // copy temp struct to public struct
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
};
