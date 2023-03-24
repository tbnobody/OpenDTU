// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>
#include "SPI.h"
#include <mcp_can.h>

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
    void init(uint8_t huawei_miso, uint8_t huawei_mosi, uint8_t huawei_clk, uint8_t huawei_irq, uint8_t huawei_cs);
    void loop();
    void setValue(float in, uint8_t parameterType);
    RectifierParameters_t& get();
    unsigned long getLastUpdate();

private:
    void sendRequest();
    void onReceive(uint8_t* frame, uint8_t len);

    unsigned long previousMillis;
    unsigned long lastUpdate;
    RectifierParameters_t _rp;

    SPIClass *hspi;
    MCP_CAN *CAN;
    uint8_t _huawei_irq;

};

extern HuaweiCanClass HuaweiCan;
