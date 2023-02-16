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
#include <map>

#ifndef VICTRON_PIN_TX
#define VICTRON_PIN_TX 21      // HardwareSerial TX Pin
#endif

#ifndef VICTRON_PIN_RX
#define VICTRON_PIN_RX 22      // HardwareSerial RX Pin
#endif



class VeDirectFrameHandler {

public:

    VeDirectFrameHandler();
    void init(int8_t rx, int8_t tx);             // initialize HardewareSerial
    void setPollInterval(unsigned long interval); // set poll intervall in seconds
    void loop();                                 // main loop to read ve.direct data
    unsigned long getLastUpdate();               // timestamp of last successful frame read
    bool isDataValid();                          // return true if data valid and not outdated
    String getPidAsString(const char* pid);      // product id as string  
    String getCsAsString(const char* cs);        // current state as string
    String getErrAsString(const char* err);      // errer state as string
    String getOrAsString(const char* offReason); // off reason as string
    String getMpptAsString(const char* mppt);    // state of mppt as string

    std::map<String, String> veMap;              // public map for received name and value pairs

private:
    void setLastUpdate();                     // set timestampt after successful frame read
    void rxData(uint8_t inbyte);              // byte of serial data
    void frameEndEvent(bool);                 // copy temp map to public map
    void logE(const char *, const char *);    
    bool hexRxEvent(uint8_t);

    //bool mStop;                               // not sure what Victron uses this for, not using

    enum States {                               // state machine
        IDLE,
        RECORD_BEGIN,
        RECORD_NAME,
        RECORD_VALUE,
        CHECKSUM,
        RECORD_HEX
    };

    int _state;                                // current state
    uint8_t	_checksum;                         // checksum value
    String _name;                              // buffer for the field name
    String _value;                             // buffer for the field value
    std::map<String, String> _tmpMap;          // private map for received name and value pairs
    unsigned long _pollInterval;
    unsigned long _lastPoll;
};

extern VeDirectFrameHandler VeDirect;

