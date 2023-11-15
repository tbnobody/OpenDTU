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

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <math.h>

HuaweiCanClass HuaweiCan;
HuaweiCanCommClass HuaweiCanComm;

// *******************************************************
// Huawei CAN Communication
// *******************************************************

// Using a C function to avoid static C++ member
void HuaweiCanCommunicationTask(void* parameter) {
  for( ;; ) {
    HuaweiCanComm.loop();
    yield();
  }
}

bool HuaweiCanCommClass::init(uint8_t huawei_miso, uint8_t huawei_mosi, uint8_t huawei_clk,
        uint8_t huawei_irq, uint8_t huawei_cs, uint32_t frequency) {
    SPI = new SPIClass(HSPI);
    SPI->begin(huawei_clk, huawei_miso, huawei_mosi, huawei_cs);
    pinMode(huawei_cs, OUTPUT);
    digitalWrite(huawei_cs, HIGH);

    pinMode(huawei_irq, INPUT_PULLUP);
    _huaweiIrq = huawei_irq;

    auto mcp_frequency = MCP_8MHZ;
    if (16000000UL == frequency) { mcp_frequency = MCP_16MHZ; }
    else if (8000000UL != frequency) {
        MessageOutput.printf("Huawei CAN: unknown frequency %d Hz, using 8 MHz\r\n", mcp_frequency);
    }

    _CAN = new MCP_CAN(SPI, huawei_cs);
    if (!_CAN->begin(MCP_STDEXT, CAN_125KBPS, mcp_frequency) == CAN_OK) {
        return false;
    }

    const uint32_t myMask = 0xFFFFFFFF;         // Look at all incoming bits and...
    const uint32_t myFilter = 0x1081407F;       // filter for this message only
    _CAN->init_Mask(0, 1, myMask);
    _CAN->init_Filt(0, 1, myFilter);
    _CAN->init_Mask(1, 1, myMask);

    // Change to normal mode to allow messages to be transmitted
    _CAN->setMode(MCP_NORMAL);

    return true;
}

// Public methods need to obtain semaphore

void HuaweiCanCommClass::loop() 
{ 
  std::lock_guard<std::mutex> lock(_mutex);
  
  INT32U rxId;
  unsigned char len = 0;
  unsigned char rxBuf[8];
  uint8_t i;

  if (!digitalRead(_huaweiIrq)) {
    // If CAN_INT pin is low, read receive buffer
    _CAN->readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
    if((rxId & 0x80000000) == 0x80000000) {   // Determine if ID is standard (11 bits) or extended (29 bits)
      if ((rxId & 0x1FFFFFFF) == 0x1081407F && len == 8) {

        uint32_t value = __bswap32(* reinterpret_cast<uint32_t*> (rxBuf + 4));

        // Input power 0x70, Input frequency 0x71, Input current 0x72
        // Output power 0x73, Efficiency 0x74, Output Voltage 0x75 and Output Current 0x76
        if(rxBuf[1] >= 0x70 && rxBuf[1] <= 0x76 ) {
          _recValues[rxBuf[1] - 0x70] = value;
        }

        // Input voltage
        if(rxBuf[1] == 0x78 ) {
          _recValues[HUAWEI_INPUT_VOLTAGE_IDX] = value;
        }

        // Output Temperature
        if(rxBuf[1] == 0x7F ) {
          _recValues[HUAWEI_OUTPUT_TEMPERATURE_IDX] = value;
        }

        // Input Temperature 0x80, Output Current 1 0x81 and Output Current 2 0x82
        if(rxBuf[1] >= 0x80 && rxBuf[1] <= 0x82 ) {
          _recValues[rxBuf[1] - 0x80 + HUAWEI_INPUT_TEMPERATURE_IDX] = value;
        }

        // This is the last value that is send
        if(rxBuf[1] == 0x81) {
          _completeUpdateReceived = true;
        }
      }
    }
    // Other emitted codes not handled here are: 0x1081407E (Ack), 0x1081807E (Ack Frame), 0x1081D27F (Description), 0x1001117E (Whr meter), 0x100011FE (unclear), 0x108111FE (output enabled), 0x108081FE (unclear). See:
    // https://github.com/craigpeacock/Huawei_R4850G2_CAN/blob/main/r4850.c
    // https://www.beyondlogic.org/review-huawei-r4850g2-power-supply-53-5vdc-3kw/
  }

  // Transmit values
  for (i = 0; i < HUAWEI_OFFLINE_CURRENT; i++) {
    if ( _hasNewTxValue[i] == true) {
      uint8_t data[8] = {0x01, i, 0x00, 0x00, 0x00, 0x00, (uint8_t)((_txValues[i] & 0xFF00) >> 8), (uint8_t)(_txValues[i] & 0xFF)};

      // Send extended message 
      byte sndStat = _CAN->sendMsgBuf(0x108180FE, 1, 8, data);
      if (sndStat == CAN_OK) {
        _hasNewTxValue[i] = false;
      } else {
        _errorCode |= HUAWEI_ERROR_CODE_TX;
      }
    }
  }

  if (_nextRequestMillis < millis()) {
    sendRequest();
    _nextRequestMillis = millis() + HUAWEI_DATA_REQUEST_INTERVAL_MS;
  }

} 

uint32_t HuaweiCanCommClass::getParameterValue(uint8_t parameter) 
{ 
  std::lock_guard<std::mutex> lock(_mutex);
  uint32_t v = 0;
  if (parameter < HUAWEI_OUTPUT_CURRENT1_IDX) {
    v =  _recValues[parameter];
  }
  return v;
}

bool HuaweiCanCommClass::gotNewRxDataFrame(bool clear) 
{ 
  std::lock_guard<std::mutex> lock(_mutex);
  bool b = false;
  b = _completeUpdateReceived;
  if (clear) {
    _completeUpdateReceived = false;
  }
  return b;
}

uint8_t HuaweiCanCommClass::getErrorCode(bool clear) 
{ 
  std::lock_guard<std::mutex> lock(_mutex);
  uint8_t e = 0;
  e = _errorCode;
  if (clear) {
    _errorCode = 0;
  }
  return e;
}

void HuaweiCanCommClass::setParameterValue(uint16_t in, uint8_t parameterType) 
{
  std::lock_guard<std::mutex> lock(_mutex);
  if (parameterType < HUAWEI_OFFLINE_CURRENT) {
    _txValues[parameterType] = in;
    _hasNewTxValue[parameterType] = true;
  }
}

// Private methods
// Requests current values from Huawei unit. Response is handled in onReceive
void HuaweiCanCommClass::sendRequest()
{
    uint8_t data[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    //Send extended message 
    byte sndStat = _CAN->sendMsgBuf(0x108040FE, 1, 8, data);
    if(sndStat != CAN_OK) {
        _errorCode |= HUAWEI_ERROR_CODE_RX;
    }
}

// *******************************************************
// Huawei CAN Controller
// *******************************************************

void HuaweiCanClass::init(uint8_t huawei_miso, uint8_t huawei_mosi, uint8_t huawei_clk, uint8_t huawei_irq, uint8_t huawei_cs, uint8_t huawei_power)
{
    if (_initialized) {
      return;
    }

    const CONFIG_T& config = Configuration.get();

    if (!config.Huawei_Enabled) {
        return;
    }

    if (!HuaweiCanComm.init(huawei_miso, huawei_mosi, huawei_clk, huawei_irq, huawei_cs, config.Huawei_CAN_Controller_Frequency)) {
      MessageOutput.println("[HuaweiCanClass::init] Error Initializing Huawei CAN communication...");
      return;
    };

    pinMode(huawei_power, OUTPUT);
    digitalWrite(huawei_power, HIGH);
    _huaweiPower = huawei_power;

    if (config.Huawei_Auto_Power_Enabled) {
      _mode = HUAWEI_MODE_AUTO_INT;
    }

    xTaskCreate(HuaweiCanCommunicationTask,"HUAWEI_CAN_0",1000,NULL,0,&_HuaweiCanCommunicationTaskHdl);

    MessageOutput.println("[HuaweiCanClass::init] MCP2515 Initialized Successfully!");
    _initialized = true;
}

RectifierParameters_t * HuaweiCanClass::get()
{
    return &_rp;
}

uint32_t HuaweiCanClass::getLastUpdate()
{
    return _lastUpdateReceivedMillis;
}

void HuaweiCanClass::processReceivedParameters()
{
    _rp.input_power = HuaweiCanComm.getParameterValue(HUAWEI_INPUT_POWER_IDX) / 1024.0;
    _rp.input_frequency = HuaweiCanComm.getParameterValue(HUAWEI_INPUT_FREQ_IDX) / 1024.0;
    _rp.input_current = HuaweiCanComm.getParameterValue(HUAWEI_INPUT_CURRENT_IDX) / 1024.0;
    _rp.output_power = HuaweiCanComm.getParameterValue(HUAWEI_OUTPUT_POWER_IDX) / 1024.0;
    _rp.efficiency = HuaweiCanComm.getParameterValue(HUAWEI_EFFICIENCY_IDX) / 1024.0;
    _rp.output_voltage = HuaweiCanComm.getParameterValue(HUAWEI_OUTPUT_VOLTAGE_IDX) / 1024.0;
    _rp.max_output_current = static_cast<float>(HuaweiCanComm.getParameterValue(HUAWEI_OUTPUT_CURRENT_MAX_IDX)) / MAX_CURRENT_MULTIPLIER;
    _rp.input_voltage = HuaweiCanComm.getParameterValue(HUAWEI_INPUT_VOLTAGE_IDX) / 1024.0;
    _rp.output_temp = HuaweiCanComm.getParameterValue(HUAWEI_OUTPUT_TEMPERATURE_IDX) / 1024.0;
    _rp.input_temp = HuaweiCanComm.getParameterValue(HUAWEI_INPUT_TEMPERATURE_IDX) / 1024.0;
    _rp.output_current = HuaweiCanComm.getParameterValue(HUAWEI_OUTPUT_CURRENT_IDX) / 1024.0;

    if (HuaweiCanComm.gotNewRxDataFrame(true)) {
      _lastUpdateReceivedMillis = millis();
    }
}


void HuaweiCanClass::loop()
{
  const CONFIG_T& config = Configuration.get();

  if (!config.Huawei_Enabled || !_initialized) {
      return;
  }

  processReceivedParameters();

  uint8_t com_error = HuaweiCanComm.getErrorCode(true);
  if (com_error && HUAWEI_ERROR_CODE_RX) {
    MessageOutput.println("[HuaweiCanClass::loop] Data request error");
  }
  if (com_error && HUAWEI_ERROR_CODE_TX) {
    MessageOutput.println("[HuaweiCanClass::loop] Data set error");    
  }

  // Print updated data
  if (HuaweiCanComm.gotNewRxDataFrame(false)) {
    MessageOutput.printf("[HuaweiCanClass::loop] In:  %.02fV, %.02fA, %.02fW\n", _rp.input_voltage, _rp.input_current, _rp.input_power);
    MessageOutput.printf("[HuaweiCanClass::loop] Out: %.02fV, %.02fA of %.02fA, %.02fW\n", _rp.output_voltage, _rp.output_current, _rp.max_output_current, _rp.output_power);
    MessageOutput.printf("[HuaweiCanClass::loop] Eff : %.01f%%, Temp in: %.01fC, Temp out: %.01fC\n", _rp.efficiency * 100, _rp.input_temp, _rp.output_temp);
  }

  // Internal PSU power pin (slot detect) control
  if (_rp.output_current > HUAWEI_AUTO_MODE_SHUTDOWN_CURRENT) {
    _outputCurrentOnSinceMillis = millis();
  }
  if (_outputCurrentOnSinceMillis + HUAWEI_AUTO_MODE_SHUTDOWN_DELAY < millis() && 
      (_mode == HUAWEI_MODE_AUTO_EXT || _mode == HUAWEI_MODE_AUTO_INT)) {
    digitalWrite(_huaweiPower, 1);
  }

  // ***********************
  // Automatic power control
  // ***********************

  if (_mode == HUAWEI_MODE_AUTO_INT ) {

    // Set voltage limit in periodic intervals
    if ( _nextAutoModePeriodicIntMillis < millis()) {
      MessageOutput.printf("[HuaweiCanClass::loop] Periodically setting voltage limit: %f \r\n", config.Huawei_Auto_Power_Voltage_Limit);
      _setValue(config.Huawei_Auto_Power_Voltage_Limit, HUAWEI_ONLINE_VOLTAGE);
      _nextAutoModePeriodicIntMillis = millis() + 60000;
    }

    // Check if we should run automatic power calculation at all. 
    // We may have set a value recently and still wait for output stabilization
    if (_autoModeBlockedTillMillis > millis()) {
      return;
    }

    // Re-enable automatic power control if the output voltage has dropped below threshold
    if(_rp.output_voltage < config.Huawei_Auto_Power_Enable_Voltage_Limit ) {
      _autoPowerEnabledCounter = 10;
    }


    // Check if inverter used by the power limiter is active
    std::shared_ptr<InverterAbstract> inverter =
        Hoymiles.getInverterByPos(config.PowerLimiter_InverterId);

    if (inverter != nullptr) {
        if(inverter->isProducing()) {
          _setValue(0.0, HUAWEI_ONLINE_CURRENT);
          // Don't run auto mode for a second now. Otherwise we may send too much over the CAN bus 
          _autoModeBlockedTillMillis = millis() + 1000;
          MessageOutput.printf("[HuaweiCanClass::loop] Inverter is active, disable\r\n");
          return;
        }
    }

    if (PowerMeter.getLastPowerMeterUpdate() > _lastPowerMeterUpdateReceivedMillis &&
        _autoPowerEnabledCounter > 0) {
        // We have received a new PowerMeter value. Also we're _autoPowerEnabled
        // So we're good to calculate a new limit

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
          _autoPowerEnabledCounter--;
          if (_autoPowerEnabledCounter == 0) {
            _autoPowerEnabled = false;
            _setValue(0, HUAWEI_ONLINE_CURRENT);
            return;
          }
        } else {
          _autoPowerEnabledCounter = 10;
        }

        // Limit power to maximum
        if (newPowerLimit > config.Huawei_Auto_Power_Upper_Power_Limit) {
          newPowerLimit = config.Huawei_Auto_Power_Upper_Power_Limit;
        }

        // Set the actual output limit
        float efficiency =  (_rp.efficiency > 0.5 ? _rp.efficiency : 1.0); 
        float outputCurrent = efficiency * (newPowerLimit / _rp.output_voltage);
        MessageOutput.printf("[HuaweiCanClass::loop] Output current %f \r\n", outputCurrent);
        _autoPowerEnabled = true;
        _setValue(outputCurrent, HUAWEI_ONLINE_CURRENT);

        // Don't run auto mode some time to allow for output stabilization after issuing a new value
        _autoModeBlockedTillMillis = millis() + 2 * HUAWEI_DATA_REQUEST_INTERVAL_MS;
      } else {
        // requested PL is below minium. Set current to 0
        _autoPowerEnabled = false;
        _setValue(0.0, HUAWEI_ONLINE_CURRENT);
      }
    }
  } 
}

void HuaweiCanClass::setValue(float in, uint8_t parameterType)
{
  if (_mode != HUAWEI_MODE_AUTO_INT) {
    _setValue(in, parameterType);
  }
}

void HuaweiCanClass::_setValue(float in, uint8_t parameterType)
{

    const CONFIG_T& config = Configuration.get();

    if (!config.Huawei_Enabled) {
        return;
    }

    uint16_t value;

    if (in < 0) {
      MessageOutput.printf("[HuaweiCanClass::_setValue]  Error: Tried to set voltage/current to negative value %f \r\n", in);
    }

    // Start PSU if needed
    if (in > HUAWEI_AUTO_MODE_SHUTDOWN_CURRENT && parameterType == HUAWEI_ONLINE_CURRENT && 
        (_mode == HUAWEI_MODE_AUTO_EXT || _mode == HUAWEI_MODE_AUTO_INT)) {
      digitalWrite(_huaweiPower, 0);
      _outputCurrentOnSinceMillis = millis();
    }

    if (parameterType == HUAWEI_OFFLINE_VOLTAGE || parameterType == HUAWEI_ONLINE_VOLTAGE) {
        value = in * 1024;
    } else if (parameterType == HUAWEI_OFFLINE_CURRENT || parameterType == HUAWEI_ONLINE_CURRENT) {
        value = in * MAX_CURRENT_MULTIPLIER;
    } else {
        return;
    }

    HuaweiCanComm.setParameterValue(value, parameterType);
}

void HuaweiCanClass::setMode(uint8_t mode) {
  const CONFIG_T& config = Configuration.get();

  if (!config.Huawei_Enabled) {
      return;
  }

  if(mode == HUAWEI_MODE_OFF) {
    digitalWrite(_huaweiPower, 1);
    _mode = HUAWEI_MODE_OFF;
  }
  if(mode == HUAWEI_MODE_ON) {
    digitalWrite(_huaweiPower, 0);
    _mode = HUAWEI_MODE_ON;
  }

  if (mode == HUAWEI_MODE_AUTO_INT && !config.Huawei_Auto_Power_Enabled ) {
    MessageOutput.println("[HuaweiCanClass::setMode] WARNING: Trying to setmode to internal automatic power control without being enabled in the UI. Ignoring command");
    return;
  }

  if (_mode == HUAWEI_MODE_AUTO_INT && mode != HUAWEI_MODE_AUTO_INT) {
    _autoPowerEnabled = false;
    _setValue(0, HUAWEI_ONLINE_CURRENT);
  }

  if(mode == HUAWEI_MODE_AUTO_EXT || mode == HUAWEI_MODE_AUTO_INT) {
    _mode = mode;
  }
}

bool HuaweiCanClass::getAutoPowerStatus() {
  return _autoPowerEnabled;
}

