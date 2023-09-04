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

#ifndef VICTRON_PIN_TX
#define VICTRON_PIN_TX 21      // HardwareSerial TX Pin
#endif

#ifndef VICTRON_PIN_RX
#define VICTRON_PIN_RX 22      // HardwareSerial RX Pin
#endif

#define VE_MAX_NAME_LEN 9   // VE.Direct Protocol: max name size is 9 including /0
#define VE_MAX_VALUE_LEN 33 // VE.Direct Protocol: max value size is 33 including /0
#define VE_MAX_HEX_LEN 100 // Maximum size of hex frame - max payload 34 byte (=68 char) + safe buffer


typedef struct {
    uint16_t PID;                   // product id
    char SER[VE_MAX_VALUE_LEN];     // serial number
    char FW[VE_MAX_VALUE_LEN];      // firmware release number
    bool LOAD;                      // virtual load output state (on if battery voltage reaches upper limit, off if battery reaches lower limit)
    uint8_t  CS;                    // current state of operation e. g. OFF or Bulk
    uint8_t ERR;                    // error code
    uint32_t OR;                    // off reason
    uint8_t  MPPT;                  // state of MPP tracker
    uint32_t HSDS;                  // day sequence number 1...365
    int32_t P;                      // battery output power in W (calculated)
    double V;                       // battery voltage in V
    double I;                       // battery current in A
    double E;                       // efficiency in percent (calculated, moving average)
    int32_t PPV;                    // panel power in W
    double VPV;                     // panel voltage in V
    double IPV;                     // panel current in A (calculated)
    double H19;                     // yield total kWh
    double H20;                     // yield today kWh
    int32_t H21;                   // maximum power today W
    double H22;                     // yield yesterday kWh
    int32_t H23;                   // maximum power yesterday W
} veStruct;

template<typename T, size_t WINDOW_SIZE>
class MovingAverage {
public:
    MovingAverage()
      : _sum(0)
      , _index(0)
      , _count(0) { }

    void addNumber(T num) {
        if (_count < WINDOW_SIZE) {
            _count++;
        } else {
            _sum -= _window[_index];
        }

        _window[_index] = num;
        _sum += num;
        _index = (_index + 1) % WINDOW_SIZE;
    }

    double getAverage() const {
        if (_count == 0) { return 0.0; }
        return static_cast<double>(_sum) / _count;
    }

private:
    std::array<T, WINDOW_SIZE> _window;
    T _sum;
    size_t _index;
    size_t _count;
};

class VeDirectFrameHandler {

public:

    VeDirectFrameHandler();
    void setVerboseLogging(bool verboseLogging);
    void init(int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging);
    void loop();                                 // main loop to read ve.direct data
    unsigned long getLastUpdate();               // timestamp of last successful frame read
    bool isDataValid();                          // return true if data valid and not outdated
    String getPidAsString(uint16_t pid);      // product id as string  
    String getCsAsString(uint8_t cs);        // current state as string
    String getErrAsString(uint8_t err);      // errer state as string
    String getOrAsString(uint32_t offReason); // off reason as string
    String getMpptAsString(uint8_t mppt);    // state of mppt as string

    veStruct veFrame{};                      // public struct for received name and value pairs

private:
    void setLastUpdate();                     // set timestampt after successful frame read
    void dumpDebugBuffer();
    void rxData(uint8_t inbyte);              // byte of serial data
    void textRxEvent(char *, char *);
    void frameEndEvent(bool);                 // copy temp struct to public struct
    int hexRxEvent(uint8_t);

    //bool mStop;                               // not sure what Victron uses this for, not using
    Print* _msgOut;
    bool _verboseLogging;
    int _state;                                // current state
    int _prevState;                            // previous state
    uint8_t	_checksum;                         // checksum value
    char * _textPointer;                       // pointer to the private buffer we're writing to, name or value
    int _hexSize;                               // length of hex buffer
    char _name[VE_MAX_VALUE_LEN];              // buffer for the field name
    char _value[VE_MAX_VALUE_LEN];             // buffer for the field value
    veStruct _tmpFrame{};                        // private struct for received name and value pairs
    MovingAverage<double, 5> _efficiency;
    std::array<uint8_t, 512> _debugBuffer;
    unsigned _debugIn;
    uint32_t _lastByteMillis;
    uint32_t _lastUpdate;
};

extern VeDirectFrameHandler VeDirect;

