#pragma once

#include <frozen/string.h>
#include <frozen/map.h>

#define VE_MAX_VALUE_LEN 33 // VE.Direct Protocol: max value size is 33 including /0
#define VE_MAX_HEX_LEN 100 // Maximum size of hex frame - max payload 34 byte (=68 char) + safe buffer

typedef struct {
    uint16_t PID = 0;               // product id
    char SER[VE_MAX_VALUE_LEN];     // serial number
    char FW[VE_MAX_VALUE_LEN];      // firmware release number
    float V = 0;                    // battery voltage in V
    float I = 0;                    // battery current in A
    float E = 0;                    // efficiency in percent (calculated, moving average)

    frozen::string const& getPidAsString() const; // product ID as string
} veStruct;

struct veMpptStruct : veStruct {
    uint8_t MPPT;                   // state of MPP tracker
    int32_t PPV;                    // panel power in W
    int32_t P;                      // battery output power in W (calculated)
    float VPV;                      // panel voltage in V
    float IPV;                      // panel current in A (calculated)
    bool LOAD;                      // virtual load output state (on if battery voltage reaches upper limit, off if battery reaches lower limit)
    uint8_t CS;                     // current state of operation e.g. OFF or Bulk
    uint8_t ERR;                    // error code
    uint32_t OR;                    // off reason
    uint32_t HSDS;                  // day sequence number 1...365
    float H19;                      // yield total kWh
    float H20;                      // yield today kWh
    int32_t H21;                    // maximum power today W
    float H22;                      // yield yesterday kWh
    int32_t H23;                    // maximum power yesterday W

    frozen::string const& getMpptAsString() const; // state of mppt as string
    frozen::string const& getCsAsString() const;   // current state as string
    frozen::string const& getErrAsString() const;  // error state as string
    frozen::string const& getOrAsString() const;   // off reason as string
};

struct veShuntStruct : veStruct {
    int32_t T;                      // Battery temperature
    bool tempPresent;               // Battery temperature sensor is attached to the shunt
    int32_t P;                      // Instantaneous power
    int32_t CE;                     // Consumed Amp Hours
    int32_t SOC;                    // State-of-charge
    uint32_t TTG;                   // Time-to-go
    bool ALARM;                     // Alarm condition active
    uint32_t AR;                    // Alarm Reason
    int32_t H1;                     // Depth of the deepest discharge
    int32_t H2;                     // Depth of the last discharge
    int32_t H3;                     // Depth of the average discharge
    int32_t H4;                     // Number of charge cycles
    int32_t H5;                     // Number of full discharges
    int32_t H6;                     // Cumulative Amp Hours drawn
    int32_t H7;                     // Minimum main (battery) voltage
    int32_t H8;                     // Maximum main (battery) voltage
    int32_t H9;                     // Number of seconds since last full charge
    int32_t H10;                    // Number of automatic synchronizations
    int32_t H11;                    // Number of low main voltage alarms
    int32_t H12;                    // Number of high main voltage alarms
    int32_t H13;                    // Number of low auxiliary voltage alarms
    int32_t H14;                    // Number of high auxiliary voltage alarms
    int32_t H15;                    // Minimum auxiliary (battery) voltage
    int32_t H16;                    // Maximum auxiliary (battery) voltage
    int32_t H17;                    // Amount of discharged energy
    int32_t H18;                    // Amount of charged energy
};
