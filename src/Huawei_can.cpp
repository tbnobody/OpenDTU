// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Malte Schmidt and others
 */
#include "Huawei_can.h"
#include "MessageOutput.h"
#include "PowerMeter.h"
#include "PowerLimiter.h"
#include "Configuration.h"
#include <SPI.h>
#include <mcp_can.h>

#include <math.h>

HuaweiCanClass HuaweiCan;

void HuaweiCanClass::init(uint8_t huawei_miso, uint8_t huawei_mosi, uint8_t huawei_clk, uint8_t huawei_irq, uint8_t huawei_cs, uint8_t huawei_power)
{
    if (_initialized) {
      return;
    }

    const CONFIG_T& config = Configuration.get();

    if (!config.Huawei_Enabled) {
        return;
    }

    spi = new SPIClass(HSPI);
    spi->begin(huawei_clk, huawei_miso, huawei_mosi, huawei_cs);
    pinMode(huawei_cs, OUTPUT);
    digitalWrite(huawei_cs, HIGH);

    pinMode(huawei_irq, INPUT_PULLUP);
    _huawei_irq = huawei_irq;

    CAN = new MCP_CAN(spi, huawei_cs);
    if (!CAN->begin(MCP_ANY, CAN_125KBPS, MCP_8MHZ) == CAN_OK) {
        MessageOutput.println("[HuaweiCanClass::init] Error Initializing MCP2515...");
        return;
    }

    MessageOutput.println("[HuaweiCanClass::init] MCP2515 Initialized Successfully!");
    _initialized = true;

    // Change to normal mode to allow messages to be transmitted
    CAN->setMode(MCP_NORMAL);

    pinMode(huawei_power, OUTPUT);
    digitalWrite(huawei_power, HIGH);
    _huawei_power = huawei_power;

    if (config.Huawei_Auto_Power_Enabled) {
      _mode = HUAWEI_MODE_AUTO_INT;
    }
}

RectifierParameters_t * HuaweiCanClass::get()
{
    return &_rp;
}

uint32_t HuaweiCanClass::getLastUpdate()
{
    return _lastUpdateReceivedMillis;
}

uint8_t data[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Requests current values from Huawei unit. Response is handled in onReceive
void HuaweiCanClass::sendRequest()
{
    // Send extended message 
    byte sndStat = CAN->sendMsgBuf(0x108040FE, 1, 8, data);
    if(sndStat != CAN_OK) {
        MessageOutput.println("[HuaweiCanClass::sendRequest] Error Sending Message...");
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
        _newOutputPowerReceived = true;
        // We'll only update last update on the important params
        _lastUpdateReceivedMillis = millis();
        break;

    case R48xx_DATA_EFFICIENCY:
        _rp.efficiency = value / 1024.0;
        break;

    case R48xx_DATA_OUTPUT_VOLTAGE:
        _rp.output_voltage = value / 1024.0;
        break;

    case R48xx_DATA_OUTPUT_CURRENT_MAX:
        _rp.max_output_current = static_cast<float>(value) / MAX_CURRENT_MULTIPLIER;
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

        if (_rp.output_current > HUAWEI_AUTO_MODE_SHUTDOWN_CURRENT) {
          _outputCurrentOnSinceMillis = millis();
        }

        /* This is normally the last parameter received. Print */
        _lastUpdateReceivedMillis = millis();  // We'll only update last update on the important params

        MessageOutput.printf("[HuaweiCanClass::onReceive] In:  %.02fV, %.02fA, %.02fW\n", _rp.input_voltage, _rp.input_current, _rp.input_power);
        MessageOutput.printf("[HuaweiCanClass::onReceive] Out: %.02fV, %.02fA of %.02fA, %.02fW\n", _rp.output_voltage, _rp.output_current, _rp.max_output_current, _rp.output_power);
        MessageOutput.printf("[HuaweiCanClass::onReceive] Eff: %.01f%%, Temp in: %.01fC, Temp out: %.01fC\n", _rp.efficiency * 100, _rp.input_temp, _rp.output_temp);

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

  if (!config.Huawei_Enabled || !_initialized) {
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

  // Request updated values in regular intervals
  if (_nextRequestMillis < millis()) {
     MessageOutput.println("[HUAWEI********************* Sending request"); 
      sendRequest();
      _nextRequestMillis = millis() + 5000;
  }

  // If the output current is low for a long time, shutdown PSU
  if (_outputCurrentOnSinceMillis + HUAWEI_AUTO_MODE_SHUTDOWN_DELAY < millis() && 
      (_mode == HUAWEI_MODE_AUTO_EXT || _mode == HUAWEI_MODE_AUTO_INT)) {
    digitalWrite(_huawei_power, 1);
  }

  // ***********************
  // Automatic power control
  // ***********************

  if (_mode == HUAWEI_MODE_AUTO_INT ) {

    // Set voltage limit in periodic intervals
    if ( _nextAutoModePeriodicIntMillis < millis()) {
      MessageOutput.printf("[HuaweiCanClass::loop] Periodically setting voltage limit: %f \r\n", config.Huawei_Auto_Power_Voltage_Limit);
      setValue(config.Huawei_Auto_Power_Voltage_Limit, HUAWEI_ONLINE_VOLTAGE);
      _nextAutoModePeriodicIntMillis = millis() + 60000;
    }

    // Re-enable automatic power control if the output voltage has dropped below threshold
    if(_rp.output_voltage < config.Huawei_Auto_Power_Enable_Voltage_Limit ) {
      _autoPowerEnabled = 10;
    }

    if ((PowerLimiter.getPowerLimiterState() == PL_UI_STATE_INACTIVE ||
        PowerLimiter.getPowerLimiterState() == PL_UI_STATE_CHARGING) && 
        PowerMeter.getLastPowerMeterUpdate() > _lastPowerMeterUpdateReceivedMillis &&
        _newOutputPowerReceived && 
        _autoPowerEnabled > 0) {
        // Power Limiter is inactive and we have received both: 
        // a new PowerMeter and a new output power value. Also we're _autoPowerEnabled
        // So we're good to calculate a new limit

      _newOutputPowerReceived = false;
      _lastPowerMeterUpdateReceivedMillis = PowerMeter.getLastPowerMeterUpdate();

      // Calculate new power limit
      float newPowerLimit = -1 * round(PowerMeter.getPowerTotal());
      newPowerLimit += _rp.output_power;
      MessageOutput.printf("[HuaweiCanClass::loop] PL: %f, OP: %f \r\n", newPowerLimit, _rp.output_power);

      if (newPowerLimit > config.Huawei_Auto_Power_Lower_Power_Limit) {

        // Check if the output power has dropped below the lower limit (i.e. the battery is full)
        // and if the PSU should be turned off. Also we use a simple counter mechanism here to be able
        // to ramp up from zero output power when starting up
        if (_rp.output_power < config.Huawei_Auto_Power_Lower_Power_Limit) {
          MessageOutput.printf("[HuaweiCanClass::loop] Power and voltage limit reached. Disabling automatic power control .... \r\n");
          _autoPowerEnabled--;
          if (_autoPowerEnabled == 0) {
            _autoPowerActive = false;
            setValue(0, HUAWEI_ONLINE_CURRENT);
            return;
          }
        } else {
          _autoPowerEnabled = 10;
        }

        // Limit power to maximum
        if (newPowerLimit > config.Huawei_Auto_Power_Upper_Power_Limit) {
          newPowerLimit = config.Huawei_Auto_Power_Upper_Power_Limit;
        }

        // Set the actual output limit
        float efficiency =  (_rp.efficiency > 0.5 ? _rp.efficiency : 1.0); 
        float outputCurrent = efficiency * (newPowerLimit / _rp.output_voltage);
        MessageOutput.printf("[HuaweiCanClass::loop] Output current %f \r\n", outputCurrent);
        _autoPowerActive = true;
        setValue(outputCurrent, HUAWEI_ONLINE_CURRENT);

        // Issue next request for updated output values in 2s to allow for output stabilization
        _nextRequestMillis = millis() + 2000;
      } else {
        // requested PL is below minium. Set current to 0
        _autoPowerActive = false;
        setValue(0.0, HUAWEI_ONLINE_CURRENT);
      }
    }
  } 
}

void HuaweiCanClass::setValue(float in, uint8_t parameterType)
{
    uint16_t value;

    if (in < 0) {
      MessageOutput.printf("[HuaweiCanClass::setValue]  Error: Tried to set voltage/current to negative value %f \r\n", in);
    }

    // Start PSU if needed
    if (in > HUAWEI_AUTO_MODE_SHUTDOWN_CURRENT && parameterType == HUAWEI_ONLINE_CURRENT && 
        (_mode == HUAWEI_MODE_AUTO_EXT || _mode == HUAWEI_MODE_AUTO_INT)) {
      digitalWrite(_huawei_power, 0);
      _outputCurrentOnSinceMillis = millis();
    }

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
    if (sndStat != CAN_OK) {
        MessageOutput.println("[HuaweiCanClass::setValue] Error Sending Message...");
    }
}

void HuaweiCanClass::setMode(uint8_t mode) {
  const CONFIG_T& config = Configuration.get();

  if(mode == HUAWEI_MODE_OFF) {
    digitalWrite(_huawei_power, 1);
    _mode = HUAWEI_MODE_OFF;
  }
  if(mode == HUAWEI_MODE_ON) {
    digitalWrite(_huawei_power, 0);
    _mode = HUAWEI_MODE_ON;
  }

  if (mode == HUAWEI_MODE_AUTO_INT && !config.Huawei_Auto_Power_Enabled ) {
    MessageOutput.println("[HuaweiCanClass::setMode] WARNING: Trying to setmode to internal automatic power control without being enabled in the UI. Ignoring command");
    return;
  }

  if (_mode == HUAWEI_MODE_AUTO_INT && mode != HUAWEI_MODE_AUTO_INT) {
    _autoPowerActive = false;
    setValue(0, HUAWEI_ONLINE_CURRENT);
  }

  if(mode == HUAWEI_MODE_AUTO_EXT || mode == HUAWEI_MODE_AUTO_INT) {
    _mode = mode;
  }
}

bool HuaweiCanClass::getAutoPowerStatus() {
  return _autoPowerActive;
}

