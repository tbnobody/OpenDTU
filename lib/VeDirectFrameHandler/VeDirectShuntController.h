#pragma once

#include <Arduino.h>
#include "VeDirectFrameHandler.h"

class VeDirectShuntController : public VeDirectFrameHandler {
public:
    VeDirectShuntController();

    void init(int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging);

    struct veShuntStruct : veStruct {
        int32_t T;                      // Battery temperature
        bool tempPresent = false;       // Battery temperature sensor is attached to the shunt
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

    veShuntStruct veFrame{};

private:
    void textRxEvent(char * name, char * value) final;
    void frameValidEvent() final;
    veShuntStruct _tmpFrame{};                        // private struct for received name and value pairs
};

extern VeDirectShuntController VeDirectShunt;
