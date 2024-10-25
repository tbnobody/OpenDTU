#pragma once

#include <frozen/string.h>
#include <frozen/map.h>
#include <Arduino.h>

#define VE_MAX_VALUE_LEN 33 // VE.Direct Protocol: max value size is 33 including /0
#define VE_MAX_HEX_LEN 100 // Maximum size of hex frame - max payload 34 byte (=68 char) + safe buffer

typedef struct {
    uint16_t productID_PID = 0;             // product id
    char serialNr_SER[VE_MAX_VALUE_LEN];    // serial number
    char firmwareVer_FW[VE_MAX_VALUE_LEN];  // firmware release number
    uint32_t batteryVoltage_V_mV = 0;       // battery voltage in mV
    int32_t batteryCurrent_I_mA = 0;        // battery current in mA (can be negative)
    float mpptEfficiency_Percent = 0;       // efficiency in percent (calculated, moving average)

    frozen::string const& getPidAsString() const; // product ID as string
    uint32_t getFwVersionAsInteger() const;
    String getFwVersionFormatted() const;
} veStruct;

struct veMpptStruct : veStruct {
    uint8_t  stateOfTracker_MPPT;       // state of MPP tracker
    uint16_t panelPower_PPV_W;          // panel power in W
    uint32_t panelVoltage_VPV_mV;       // panel voltage in mV
    uint32_t panelCurrent_mA;           // panel current in mA (calculated)
    int16_t  batteryOutputPower_W;      // battery output power in W (calculated, can be negative if load output is used)
    uint32_t loadCurrent_IL_mA;         // Load current in mA (Available only for models with a load output)
    bool     loadOutputState_LOAD;      // virtual load output state (on if battery voltage reaches upper limit, off if battery reaches lower limit)
    uint8_t  currentState_CS;           // current state of operation e.g. OFF or Bulk
    uint8_t  errorCode_ERR;             // error code
    uint32_t offReason_OR;              // off reason
    uint16_t daySequenceNr_HSDS;        // day sequence number 1...365
    uint32_t yieldTotal_H19_Wh;         // yield total resetable Wh
    uint32_t yieldToday_H20_Wh;         // yield today Wh
    uint16_t maxPowerToday_H21_W;       // maximum power today W
    uint32_t yieldYesterday_H22_Wh;     // yield yesterday Wh
    uint16_t maxPowerYesterday_H23_W;   // maximum power yesterday W

    // these are values communicated through the HEX protocol. the pair's first
    // value is the timestamp the respective info was last received. if it is
    // zero, the value is deemed invalid. the timestamp is reset if no current
    // value could be retrieved.
    std::pair<uint32_t, int32_t> MpptTemperatureMilliCelsius;
    std::pair<uint32_t, int32_t> SmartBatterySenseTemperatureMilliCelsius;
    std::pair<uint32_t, uint32_t> NetworkTotalDcInputPowerMilliWatts;
    std::pair<uint32_t, uint32_t> BatteryAbsorptionMilliVolt;
    std::pair<uint32_t, uint32_t> BatteryFloatMilliVolt;
    std::pair<uint32_t, uint8_t> NetworkInfo;
    std::pair<uint32_t, uint8_t> NetworkMode;
    std::pair<uint32_t, uint8_t> NetworkStatus;

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
    uint16_t alarmReason_AR;        // Alarm Reason
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
    int32_t VM;                     // Mid-point voltage of the battery bank
    int32_t DM;                     // Mid-point deviation of the battery bank
    int8_t dcMonitorMode_MON;       // DC monitor mode
};

enum class VeDirectHexCommand : uint8_t {
    ENTER_BOOT = 0x0,
    PING = 0x1,
    RSV1 = 0x2,
    APP_VERSION = 0x3,
    PRODUCT_ID = 0x4,
    RSV2 = 0x5,
    RESTART = 0x6,
    GET = 0x7,
    SET = 0x8,
    RSV3 = 0x9,
    ASYNC = 0xA,
    RSV4 = 0xB,
    RSV5 = 0xC,
    RSV6 = 0xD,
    RSV7 = 0xE,
    RSV8 = 0xF
};

enum class VeDirectHexResponse : uint8_t {
    DONE = 0x1,
    UNKNOWN = 0x3,
    ERROR = 0x4,
    PING = 0x5,
    GET = 0x7,
    SET = 0x8,
    ASYNC = 0xA
};

enum class VeDirectHexRegister : uint16_t {
    DeviceMode = 0x0200,
    DeviceState = 0x0201,
    RemoteControlUsed = 0x0202,
    PanelVoltage = 0xEDBB,
    PanelPower = 0xEDBC,
    ChargerVoltage = 0xEDD5,
    ChargerCurrent = 0xEDD7,
    NetworkTotalDcInputPower = 0x2027,
    ChargeControllerTemperature = 0xEDDB,
    SmartBatterySenseTemperature = 0xEDEC,
    NetworkInfo = 0x200D,
    NetworkMode = 0x200E,
    NetworkStatus = 0x200F,
    HistoryTotal = 0x104F,
    HistoryMPPTD30 = 0x10BE,
    BatteryAbsorptionVoltage = 0xEDF7,
    BatteryFloatVoltage = 0xEDF6,
    TotalChargeCurrent = 0x2013,
    ChargeStateElapsedTime= 0x2007,
    BatteryVoltageSense = 0x2002,
    LoadCurrent = 0xEDAD,
    LoadOutputVoltage = 0xEDA9
};

struct VeDirectHexData {
    VeDirectHexResponse rsp;        // hex response code
    VeDirectHexRegister addr;       // register address
    uint8_t flags;                  // flags
    uint32_t value;                 // integer value of register
    char text[VE_MAX_HEX_LEN];      // text/string response

    frozen::string const& getResponseAsString() const;
    frozen::string const& getRegisterAsString() const;
};
