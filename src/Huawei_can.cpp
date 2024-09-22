// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Malte Schmidt and others
 */
#include "Battery.h"
#include "Huawei_can.h"
#include "MessageOutput.h"
#include "PowerMeter.h"
#include "PowerLimiter.h"
#include "Configuration.h"
#include "Battery.h"
#include "SpiManager.h"
#include <mcp_can.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <algorithm>
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

    auto spi_bus = SpiManagerInst.claim_bus_arduino();
    if (!spi_bus) { return false; }

    SPI = new SPIClass(*spi_bus);

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

void HuaweiCanClass::init(Scheduler& scheduler, uint8_t huawei_miso, uint8_t huawei_mosi, uint8_t huawei_clk, uint8_t huawei_irq, uint8_t huawei_cs, uint8_t huawei_power)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&HuaweiCanClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    this->updateSettings(huawei_miso, huawei_mosi, huawei_clk, huawei_irq, huawei_cs, huawei_power);
}

void HuaweiCanClass::updateSettings(uint8_t huawei_miso, uint8_t huawei_mosi, uint8_t huawei_clk, uint8_t huawei_irq, uint8_t huawei_cs, uint8_t huawei_power)
{
    if (_initialized) {
      return;
    }

    const CONFIG_T& config = Configuration.get();

    if (!config.Huawei.Enabled) {
        return;
    }

    if (!HuaweiCanComm.init(huawei_miso, huawei_mosi, huawei_clk, huawei_irq, huawei_cs, config.Huawei.CAN_Controller_Frequency)) {
      MessageOutput.println("[HuaweiCanClass::init] Error Initializing Huawei CAN communication...");
      return;
    };

    pinMode(huawei_power, OUTPUT);
    digitalWrite(huawei_power, HIGH);
    _huaweiPower = huawei_power;

    if (config.Huawei.Auto_Power_Enabled) {
      _mode = HUAWEI_MODE_AUTO_INT;
    }

    xTaskCreate(HuaweiCanCommunicationTask, "HUAWEI_CAN_0", 2048/*stack size*/,
        NULL/*params*/, 0/*prio*/, &_HuaweiCanCommunicationTaskHdl);

    MessageOutput.println("[HuaweiCanClass::init] MCP2515 Initialized Successfully!");
    _initialized = true;
}

RectifierParameters_t * HuaweiCanClass::get()
{
    return &_rp;
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

  if (!config.Huawei.Enabled || !_initialized) {
      return;
  }

  bool verboseLogging = config.Huawei.VerboseLogging;

  processReceivedParameters();

  uint8_t com_error = HuaweiCanComm.getErrorCode(true);
  if (com_error & HUAWEI_ERROR_CODE_RX) {
    MessageOutput.println("[HuaweiCanClass::loop] Data request error");
  }
  if (com_error & HUAWEI_ERROR_CODE_TX) {
    MessageOutput.println("[HuaweiCanClass::loop] Data set error");
  }

  // Print updated data
  if (HuaweiCanComm.gotNewRxDataFrame(false) && verboseLogging) {
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


  if (_mode == HUAWEI_MODE_AUTO_INT || _batteryEmergencyCharging) {

    // Set voltage limit in periodic intervals if we're in auto mode or if emergency battery charge is requested.
    if ( _nextAutoModePeriodicIntMillis < millis()) {
      MessageOutput.printf("[HuaweiCanClass::loop] Periodically setting voltage limit: %f \r\n", config.Huawei.Auto_Power_Voltage_Limit);
      _setValue(config.Huawei.Auto_Power_Voltage_Limit, HUAWEI_ONLINE_VOLTAGE);
      _nextAutoModePeriodicIntMillis = millis() + 60000;
    }
  }
  // ***********************
  // Emergency charge
  // ***********************
  auto stats = Battery.getStats();
  if (config.Huawei.Emergency_Charge_Enabled && stats->getImmediateChargingRequest()) {
    _batteryEmergencyCharging = true;

    // Set output current
    float efficiency =  (_rp.efficiency > 0.5 ? _rp.efficiency : 1.0);
    float outputCurrent = efficiency * (config.Huawei.Auto_Power_Upper_Power_Limit / _rp.output_voltage);
    MessageOutput.printf("[HuaweiCanClass::loop] Emergency Charge Output current %f \r\n", outputCurrent);
    _setValue(outputCurrent, HUAWEI_ONLINE_CURRENT);
    return;
  }

  if (_batteryEmergencyCharging && !stats->getImmediateChargingRequest()) {
    // Battery request has changed. Set current to 0, wait for PSU to respond and then clear state
    _setValue(0, HUAWEI_ONLINE_CURRENT);
    if (_rp.output_current < 1) {
      _batteryEmergencyCharging = false;
    }
    return;
  }

  // ***********************
  // Automatic power control
  // ***********************

  if (_mode == HUAWEI_MODE_AUTO_INT ) {

    // Check if we should run automatic power calculation at all.
    // We may have set a value recently and still wait for output stabilization
    if (_autoModeBlockedTillMillis > millis()) {
      return;
    }

    // Re-enable automatic power control if the output voltage has dropped below threshold
    if(_rp.output_voltage < config.Huawei.Auto_Power_Enable_Voltage_Limit ) {
      _autoPowerEnabledCounter = 10;
    }

    if (PowerLimiter.isGovernedInverterProducing()) {
      _setValue(0.0, HUAWEI_ONLINE_CURRENT);
      // Don't run auto mode for a second now. Otherwise we may send too much over the CAN bus
      _autoModeBlockedTillMillis = millis() + 1000;
      MessageOutput.printf("[HuaweiCanClass::loop] Inverter is active, disable\r\n");
      return;
    }

    if (PowerMeter.getLastUpdate() > _lastPowerMeterUpdateReceivedMillis &&
        _autoPowerEnabledCounter > 0) {
        // We have received a new PowerMeter value. Also we're _autoPowerEnabled
        // So we're good to calculate a new limit

      _lastPowerMeterUpdateReceivedMillis = PowerMeter.getLastUpdate();

      // Calculate new power limit
      float newPowerLimit = -1 * round(PowerMeter.getPowerTotal());
      float efficiency =  (_rp.efficiency > 0.5 ? _rp.efficiency : 1.0);

      // Powerlimit is the requested output power + permissable Grid consumption factoring in the efficiency factor
      newPowerLimit += _rp.output_power + config.Huawei.Auto_Power_Target_Power_Consumption / efficiency;

      if (verboseLogging){
        MessageOutput.printf("[HuaweiCanClass::loop] newPowerLimit: %f, output_power: %f \r\n", newPowerLimit, _rp.output_power);
      }

      // Check whether the battery SoC limit setting is enabled
      if (config.Battery.Enabled && config.Huawei.Auto_Power_BatterySoC_Limits_Enabled) {
        uint8_t _batterySoC = Battery.getStats()->getSoC();
        // Sets power limit to 0 if the BMS reported SoC reaches or exceeds the user configured value
        if (_batterySoC >= config.Huawei.Auto_Power_Stop_BatterySoC_Threshold) {
          newPowerLimit = 0;
          if (verboseLogging) {
            MessageOutput.printf("[HuaweiCanClass::loop] Current battery SoC %i reached "
                    "stop threshold %i, set newPowerLimit to %f \r\n", _batterySoC,
                    config.Huawei.Auto_Power_Stop_BatterySoC_Threshold, newPowerLimit);
          }
        }
      }

      if (newPowerLimit > config.Huawei.Auto_Power_Lower_Power_Limit) {

        // Check if the output power has dropped below the lower limit (i.e. the battery is full)
        // and if the PSU should be turned off. Also we use a simple counter mechanism here to be able
        // to ramp up from zero output power when starting up
        if (_rp.output_power < config.Huawei.Auto_Power_Lower_Power_Limit) {
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
        if (newPowerLimit > config.Huawei.Auto_Power_Upper_Power_Limit) {
          newPowerLimit = config.Huawei.Auto_Power_Upper_Power_Limit;
        }

        // Calculate output current
        float calculatedCurrent = efficiency * (newPowerLimit / _rp.output_voltage);

        // Limit output current to value requested by BMS
        float permissableCurrent = stats->getChargeCurrentLimitation() - (stats->getChargeCurrent() - _rp.output_current); // BMS current limit - current from other sources, e.g. Victron MPPT charger
        float outputCurrent = std::min(calculatedCurrent, permissableCurrent);
        outputCurrent= outputCurrent > 0 ? outputCurrent : 0;

        if (verboseLogging) {
            MessageOutput.printf("[HuaweiCanClass::loop] Setting output current to %.2fA. This is the lower value of calculated %.2fA and BMS permissable %.2fA currents\r\n", outputCurrent, calculatedCurrent, permissableCurrent);
        }
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

    if (!config.Huawei.Enabled) {
        return;
    }

    uint16_t value;

    if (in < 0) {
      MessageOutput.printf("[HuaweiCanClass::_setValue]  Error: Tried to set voltage/current to negative value %f \r\n", in);
      return;
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

  if (!config.Huawei.Enabled) {
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

  if (mode == HUAWEI_MODE_AUTO_INT && !config.Huawei.Auto_Power_Enabled ) {
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



