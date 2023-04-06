// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Malte Schmidt and others
 */
#include "Huawei_can.h"
#include "MessageOutput.h"
#include "Configuration.h"
#include <SPI.h>
#include <mcp_can.h>

#include <math.h>

HuaweiCanClass HuaweiCan;

void HuaweiCanClass::init(uint8_t huawei_miso, uint8_t huawei_mosi, uint8_t huawei_clk, uint8_t huawei_irq, uint8_t huawei_cs, uint8_t huawei_power)
{
    initialized = false;

    const CONFIG_T& config = Configuration.get();

    if (!config.Huawei_Enabled) {
        return;
    }

    spi = new SPIClass(VSPI);
    spi->begin(huawei_clk, huawei_miso, huawei_mosi, huawei_cs);
    pinMode(huawei_cs, OUTPUT);
    digitalWrite(huawei_cs, HIGH);

    pinMode(huawei_irq, INPUT_PULLUP);
    _huawei_irq = huawei_irq;

    CAN = new MCP_CAN(spi, huawei_cs);
    if (!CAN->begin(MCP_ANY, CAN_125KBPS, MCP_8MHZ) == CAN_OK) {
        MessageOutput.println("Error Initializing MCP2515...");
        return;
    }

    MessageOutput.println("MCP2515 Initialized Successfully!");
    initialized = true;

    // Change to normal mode to allow messages to be transmitted
    CAN->setMode(MCP_NORMAL);

    pinMode(huawei_power, OUTPUT);
    digitalWrite(huawei_power, HIGH);
    _huawei_power = huawei_power;
}

RectifierParameters_t * HuaweiCanClass::get()
{
    return &_rp;
}

uint32_t HuaweiCanClass::getLastUpdate()
{
    return lastUpdate;
}
uint8_t data[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Requests current values from Huawei unit. Response is handled in onReceive
void HuaweiCanClass::sendRequest()
{
    if (previousMillis < millis()) {
        // Send extended message 
        byte sndStat = CAN->sendMsgBuf(0x108040FE, 1, 8, data);
        if(sndStat == CAN_OK) {
            MessageOutput.println("Message Sent Successfully!");
        } else {
            MessageOutput.println("Error Sending Message...");
        }

        previousMillis += 5000;
    }
}

void HuaweiCanClass::onReceive(uint8_t* frame, uint8_t len)
{
    if (len != 8) {
      return;
    }

    uint32_t value = __bswap32(* reinterpret_cast<uint32_t*> (frame + 4));

    switch (frame[1]) {
    case R48xx_DATA_INPUT_POWER:
        _rp.input_power = value / 1024.0;
        break;

    case R48xx_DATA_INPUT_FREQ:
        _rp.input_frequency = value / 1024.0;
        break;

    case R48xx_DATA_INPUT_CURRENT:
        _rp.input_current = value / 1024.0;
        break;

    case R48xx_DATA_OUTPUT_POWER:
        _rp.output_power = value / 1024.0;
        // We'll only update last update on the important params
        lastUpdate = millis();
        break;

    case R48xx_DATA_EFFICIENCY:
        _rp.efficiency = value / 1024.0;
        break;

    case R48xx_DATA_OUTPUT_VOLTAGE:
        _rp.output_voltage = value / 1024.0;
        break;

    case R48xx_DATA_OUTPUT_CURRENT_MAX:
        _rp.max_output_current = value / MAX_CURRENT_MULTIPLIER;
        break;

    case R48xx_DATA_INPUT_VOLTAGE:
        _rp.input_voltage = value / 1024.0;
        break;

    case R48xx_DATA_OUTPUT_TEMPERATURE:
        _rp.output_temp = value / 1024.0;
        break;

    case R48xx_DATA_INPUT_TEMPERATURE:
        _rp.input_temp = value / 1024.0;
        break;

    case R48xx_DATA_OUTPUT_CURRENT1:
        // printf("Output Current(1) %.02fA\r\n", value / 1024.0);
        // output_current = value / 1024.0;
        break;

    case R48xx_DATA_OUTPUT_CURRENT:
        _rp.output_current = value / 1024.0;

        /* This is normally the last parameter received. Print */
        lastUpdate = millis();  // We'll only update last update on the important params

        MessageOutput.printf("In:  %.02fV, %.02fA, %.02fW\n", _rp.input_voltage, _rp.input_current, _rp.input_power);
        MessageOutput.printf("Out: %.02fV, %.02fA of %.02fA, %.02fW\n", _rp.output_voltage, _rp.output_current, _rp.max_output_current, _rp.output_power);
        MessageOutput.printf("Eff: %.01f%%, Temp in: %.01fC, Temp out: %.01fC\n", _rp.efficiency * 100, _rp.input_temp, _rp.output_temp);

        break;

    default:
        // printf("Unknown parameter 0x%02X, 0x%04X\r\n",frame[1], value);
        break;
    }
}

void HuaweiCanClass::loop()
{

  INT32U rxId;
  unsigned char len = 0;
  unsigned char rxBuf[8];

  const CONFIG_T& config = Configuration.get();

  if (!config.Huawei_Enabled || !initialized) {
      return;
  }

  if (!digitalRead(_huawei_irq)) {
    // If CAN_INT pin is low, read receive buffer
    CAN->readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
    
    if((rxId & 0x80000000) == 0x80000000) {  // Determine if ID is standard (11 bits) or extended (29 bits)
      // MessageOutput.printf("Extended ID: 0x%.8lX  DLC: %1d  \n", (rxId & 0x1FFFFFFF), len);
      if ((rxId & 0x1FFFFFFF) == 0x1081407F) {
        onReceive(rxBuf, len);
      }
      // Other emitted codes not handled here are: 0x1081407E, 0x1081807E, 0x1081D27F, 0x1001117E, 0x100011FE, 0x108111FE, 0x108081FE. See:
      // https://github.com/craigpeacock/Huawei_R4850G2_CAN/blob/main/r4850.c
      // https://www.beyondlogic.org/review-huawei-r4850g2-power-supply-53-5vdc-3kw/
    }    
  }
    sendRequest();
}

void HuaweiCanClass::setValue(float in, uint8_t parameterType)
{
    uint16_t value;
    if (parameterType == HUAWEI_OFFLINE_VOLTAGE || parameterType == HUAWEI_ONLINE_VOLTAGE) {
        value = in * 1024;
    } else if (parameterType == HUAWEI_OFFLINE_CURRENT || parameterType == HUAWEI_ONLINE_CURRENT) {
        value = in * MAX_CURRENT_MULTIPLIER;
    } else {
        return;
    }

    uint8_t data[8] = {0x01, parameterType, 0x00, 0x00, 0x00, 0x00, (uint8_t)((value & 0xFF00) >> 8), (uint8_t)(value & 0xFF)};

    // Send extended message 
    byte sndStat = CAN->sendMsgBuf(0x108180FE, 1, 8, data);
    if (sndStat == CAN_OK) {
        MessageOutput.println("Message Sent Successfully!");
    } else {
        MessageOutput.println("Error Sending Message...");
    }
}

void HuaweiCanClass::setPower(bool power) {
    digitalWrite(_huawei_power, !power);
}
