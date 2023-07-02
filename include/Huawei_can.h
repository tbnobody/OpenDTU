// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>
#include "SPI.h"
#include <mcp_can.h>

#ifndef HUAWEI_PIN_MISO
#define HUAWEI_PIN_MISO 12
#endif

#ifndef HUAWEI_PIN_MOSI
#define HUAWEI_PIN_MOSI 13
#endif

#ifndef HUAWEI_PIN_SCLK
#define HUAWEI_PIN_SCLK 26
#endif

#ifndef HUAWEI_PIN_IRQ
#define HUAWEI_PIN_IRQ 25
#endif

#ifndef HUAWEI_PIN_CS
#define HUAWEI_PIN_CS 15
#endif

#ifndef HUAWEI_PIN_POWER
#define HUAWEI_PIN_POWER 33
#endif

#define HUAWEI_MINIMAL_OFFLINE_VOLTAGE 48
#define HUAWEI_MINIMAL_ONLINE_VOLTAGE 42

#define MAX_CURRENT_MULTIPLIER 20

#define HUAWEI_OFFLINE_VOLTAGE 0x01
#define HUAWEI_ONLINE_VOLTAGE 0x00
#define HUAWEI_OFFLINE_CURRENT 0x04
#define HUAWEI_ONLINE_CURRENT 0x03

#define R48xx_DATA_INPUT_POWER 0x70
#define R48xx_DATA_INPUT_FREQ 0x71
#define R48xx_DATA_INPUT_CURRENT 0x72
#define R48xx_DATA_OUTPUT_POWER 0x73
#define R48xx_DATA_EFFICIENCY 0x74
#define R48xx_DATA_OUTPUT_VOLTAGE 0x75
#define R48xx_DATA_OUTPUT_CURRENT_MAX 0x76
#define R48xx_DATA_INPUT_VOLTAGE 0x78
#define R48xx_DATA_OUTPUT_TEMPERATURE 0x7F
#define R48xx_DATA_INPUT_TEMPERATURE 0x80
#define R48xx_DATA_OUTPUT_CURRENT 0x81
#define R48xx_DATA_OUTPUT_CURRENT1 0x82

#define HUAWEI_MODE_OFF 0
#define HUAWEI_MODE_ON 1
#define HUAWEI_MODE_AUTO_EXT 2
#define HUAWEI_MODE_AUTO_INT 3

// Wait time/current before shuting down the PSU / charger
// This is set to allow the fan to run for some time
#define HUAWEI_AUTO_MODE_SHUTDOWN_DELAY 60000
#define HUAWEI_AUTO_MODE_SHUTDOWN_CURRENT 1.0

struct RectifierParameters_t {
    float input_voltage;
    float input_frequency;
    float input_current;
    float input_power;
    float input_temp;
    float efficiency;
    float output_voltage;
    float output_current;
    float max_output_current;
    float output_power;
    float output_temp;
    float amp_hour;
};

class HuaweiCanClass {
public:
    void init(uint8_t huawei_miso, uint8_t huawei_mosi, uint8_t huawei_clk, uint8_t huawei_irq, uint8_t huawei_cs, uint8_t huawei_power);
    void loop();
    void setValue(float in, uint8_t parameterType);
    void setMode(uint8_t mode);

    RectifierParameters_t * get();
    uint32_t getLastUpdate();
    bool getAutoPowerStatus();

private:
    void sendRequest();
    void onReceive(uint8_t* frame, uint8_t len);

    SPIClass *spi;
    MCP_CAN *CAN;
    bool    _initialized = false;
    uint8_t _huawei_irq;             // IRQ pin
    uint8_t _huawei_power;           // Power pin
    uint8_t _mode = HUAWEI_MODE_AUTO_EXT;

    RectifierParameters_t _rp;

    uint32_t _lastUpdateReceivedMillis;           // Timestamp for last data seen from the PSU
    uint32_t _nextRequestMillis = 0;              // When to send next data request to PSU 
    uint32_t _nextAutoModePeriodicIntMillis;      // When to send the next output volume request in Automatic mode
    uint32_t _lastPowerMeterUpdateReceivedMillis; // Timestamp of last power meter value
    uint32_t _outputCurrentOnSinceMillis;         // Timestamp since when the PSU was idle at zero amps
    bool _newOutputPowerReceived = false;
    uint8_t _autoPowerEnabled = false;
    bool _autoPowerActive = false;
};

extern HuaweiCanClass HuaweiCan;
