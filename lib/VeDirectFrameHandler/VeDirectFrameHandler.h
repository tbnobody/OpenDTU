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

#ifndef VICTRON_PIN_TX
#define VICTRON_PIN_TX 21      // HardwareSerial TX Pin
#endif

#ifndef VICTRON_PIN_RX
#define VICTRON_PIN_RX 22      // HardwareSerial RX Pin
#endif

#define VE_MAX_NAME_LEN 9   // VE.Direct Protocol: max name size is 9 including /0
#define VE_MAX_VALUE_LEN 33 // VE.Direct Protocol: max value size is 33 including /0

typedef struct {
    uint16_t PID;                   // pruduct id
    char SER[VE_MAX_VALUE_LEN];     // serial number
    char FW[VE_MAX_VALUE_LEN];      // firmware release number
    bool LOAD;                      // virtual load output state (on if battery voltage reaches upper limit, off if battery reaches lower limit)
    uint8_t  CS;                    // current state of operation e. g. OFF or Bulk
    uint8_t ERR;                    // error code
    uint32_t OR;                    // off reason
    uint8_t  MPPT;                  // state of MPP tracker
    uint32_t HSDS;                  // day sequence number 1...365
    double V;                       // battery voltage in V
    double I;                       // battery current in A
    double VPV;                     // panel voltage in V
    uint32_t PPV;                    // panel power in W
    double H19;                     // yield total kWh
    double H20;                     // yield today kWh
    uint32_t H21;                   // maximum power today W
    double H22;                     // yield yesterday kWh
    uint32_t H23;                   // maximum power yesterday W
} veStruct;

class VeDirectFrameHandler {

public:

    VeDirectFrameHandler();
    void init(int8_t rx, int8_t tx);             // initialize HardewareSerial
    void setPollInterval(unsigned long interval); // set poll intervall in seconds
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
    void rxData(uint8_t inbyte);              // byte of serial data
    void textRxEvent(char *, char *);
    void frameEndEvent(bool);                 // copy temp struct to public struct
    void logE(const char *, const char *);    
    bool hexRxEvent(uint8_t);

    //bool mStop;                               // not sure what Victron uses this for, not using
    int _state;                                // current state
    uint8_t	_checksum;                         // checksum value
    char * _textPointer;                       // pointer to the private buffer we're writing to, name or value
    char _name[VE_MAX_VALUE_LEN];              // buffer for the field name
    char _value[VE_MAX_VALUE_LEN];             // buffer for the field value
    veStruct _tmpFrame{};                        // private struct for received name and value pairs
    unsigned long _pollInterval;
    unsigned long _lastPoll;
};

extern VeDirectFrameHandler VeDirect;

