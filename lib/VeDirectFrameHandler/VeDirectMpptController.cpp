/* VeDirectMpptController.cpp
 *
 *
 * 2020.08.20 - 0.0 - ???
 * 2024.03.18 - 0.1 - add of: - temperature from "Smart Battery Sense" connected over VE.Smart network
 * 					  		  - temperature from internal MPPT sensor
 * 					  		  - "total DC input power" from MPPT's connected over VE.Smart network
 */

#include <Arduino.h>
#include "VeDirectMpptController.h"

//#define PROCESS_NETWORK_STATE

void VeDirectMpptController::init(int8_t rx, int8_t tx, Print* msgOut,
		bool verboseLogging, uint8_t hwSerialPort)
{
	VeDirectFrameHandler::init("MPPT", rx, tx, msgOut,
			verboseLogging, hwSerialPort);
}

bool VeDirectMpptController::processTextDataDerived(std::string const& name, std::string const& value)
{
	if (name == "IL") {
		_tmpFrame.loadCurrent_IL_mA.second = atol(value.c_str());
        _tmpFrame.loadCurrent_IL_mA.first = millis();
		return true;
	}
	if (name == "LOAD") {
		_tmpFrame.loadOutputState_LOAD.second = (value == "ON");
        _tmpFrame.loadOutputState_LOAD.first = millis();
		return true;
	}
    if (name == "RELAY") {
		_tmpFrame.relayState_RELAY.second = (value == "ON");
        _tmpFrame.relayState_RELAY.first = millis();
		return true;
	}
	if (name == "CS") {
		_tmpFrame.currentState_CS = atoi(value.c_str());
		return true;
	}
	if (name == "ERR") {
		_tmpFrame.errorCode_ERR = atoi(value.c_str());
		return true;
	}
	if (name == "OR") {
		_tmpFrame.offReason_OR = strtol(value.c_str(), nullptr, 0);
		return true;
	}
	if (name == "MPPT") {
		_tmpFrame.stateOfTracker_MPPT = atoi(value.c_str());
		return true;
	}
	if (name == "HSDS") {
		_tmpFrame.daySequenceNr_HSDS = atoi(value.c_str());
		return true;
	}
	if (name == "VPV") {
		_tmpFrame.panelVoltage_VPV_mV = atol(value.c_str());
		return true;
	}
	if (name == "PPV") {
		_tmpFrame.panelPower_PPV_W = atoi(value.c_str());
		return true;
	}
	if (name == "H19") {
		_tmpFrame.yieldTotal_H19_Wh = atol(value.c_str()) * 10;
		return true;
	}
	if (name == "H20") {
		_tmpFrame.yieldToday_H20_Wh = atol(value.c_str()) * 10;
		return true;
	}
	if (name == "H21") {
		_tmpFrame.maxPowerToday_H21_W = atoi(value.c_str());
		return true;
	}
	if (name == "H22") {
		_tmpFrame.yieldYesterday_H22_Wh = atol(value.c_str()) * 10;
		return true;
	}
	if (name == "H23") {
		_tmpFrame.maxPowerYesterday_H23_W = atoi(value.c_str());
		return true;
	}

	return false;
}

/*
 *  frameValidEvent
 *  This function is called at the end of the received frame.
 */
void VeDirectMpptController::frameValidEvent() {
	// power into the battery, (+) means charging, (-) means discharging
	_tmpFrame.batteryOutputPower_W = static_cast<int16_t>((_tmpFrame.batteryVoltage_V_mV / 1000.0f) * (_tmpFrame.batteryCurrent_I_mA / 1000.0f));

	// calculation of the panel current
	if ((_tmpFrame.panelVoltage_VPV_mV > 0) && (_tmpFrame.panelPower_PPV_W >= 1)) {
		_tmpFrame.panelCurrent_mA = static_cast<uint32_t>(_tmpFrame.panelPower_PPV_W * 1000000.0f / _tmpFrame.panelVoltage_VPV_mV);
	} else {
		_tmpFrame.panelCurrent_mA = 0;
	}

	// calculation of the MPPT efficiency
    float loadCurrent = (_tmpFrame.loadCurrent_IL_mA.first > 0) ? _tmpFrame.loadCurrent_IL_mA.second / 1000.0f : 0.0f;
	float totalPower_W = (loadCurrent + _tmpFrame.batteryCurrent_I_mA / 1000.0f) * _tmpFrame.batteryVoltage_V_mV / 1000.0f;
	if (_tmpFrame.panelPower_PPV_W > 0) {
		_efficiency.addNumber(totalPower_W * 100.0f / _tmpFrame.panelPower_PPV_W);
		_tmpFrame.mpptEfficiency_Percent = _efficiency.getAverage();
	} else {
		_tmpFrame.mpptEfficiency_Percent = 0.0f;
	}
}


void VeDirectMpptController::loop()
{
	// First we send HEX-Commands (timing improvement)
	if (isHexCommandPossible()) {
		sendNextHexCommandFromQueue();
	}

	// Second we read Text- and HEX-Messages
	VeDirectFrameHandler::loop();

	// Note: Room for improvement, longer data valid time for slow changing values?
	auto resetTimestamp = [this](auto& pair) {
		if (pair.first > 0 && (millis() - pair.first) > (10 * 1000)) {
			pair.first = 0;
		}
	};

    // Check if optional TEXT-Data is outdated
    resetTimestamp(_tmpFrame.loadOutputState_LOAD);
	resetTimestamp(_tmpFrame.loadCurrent_IL_mA);
    resetTimestamp(_tmpFrame.relayState_RELAY);

	// Third we check if HEX-Data is outdated
    if (!isHexCommandPossible()) { return; }
	resetTimestamp(_tmpFrame.MpptTemperatureMilliCelsius);
	resetTimestamp(_tmpFrame.SmartBatterySenseTemperatureMilliCelsius);
	resetTimestamp(_tmpFrame.NetworkTotalDcInputPowerMilliWatts);
	resetTimestamp(_tmpFrame.BatteryFloatMilliVolt);
	resetTimestamp(_tmpFrame.BatteryAbsorptionMilliVolt);

#ifdef PROCESS_NETWORK_STATE
	resetTimestamp(_tmpFrame.NetworkInfo);
	resetTimestamp(_tmpFrame.NetworkMode);
	resetTimestamp(_tmpFrame.NetworkStatus);
#endif // PROCESS_NETWORK_STATE
}


/*
 * hexDataHandler()
 * analyze the content of VE.Direct hex messages
 * handles the received hex data from the MPPT
 */
bool VeDirectMpptController::hexDataHandler(VeDirectHexData const &data) {
	if (data.rsp != VeDirectHexResponse::GET &&
			data.rsp != VeDirectHexResponse::ASYNC) { return false; }

	auto regLog = static_cast<uint16_t>(data.addr);

	// we check whether the answer matches a previously asked query
	if ((data.rsp == VeDirectHexResponse::GET) && (data.addr == _hexQueue[_sendQueueNr]._hexRegister)) {
		_sendTimeout = 0;
	}

	switch (data.addr) {
		case VeDirectHexRegister::ChargeControllerTemperature:
			_tmpFrame.MpptTemperatureMilliCelsius =
				{ millis(), static_cast<int32_t>(data.value) * 10 };

			if (_verboseLogging) {
				_msgOut->printf("%s Hex Data: MPPT Temperature (0x%04X): %.2f°C\r\n",
						_logId, regLog,
						_tmpFrame.MpptTemperatureMilliCelsius.second / 1000.0);
			}
			return true;
			break;

		case VeDirectHexRegister::SmartBatterySenseTemperature:
			if (data.value == 0xFFFF) {
				if (_verboseLogging) {
					_msgOut->printf("%s Hex Data: Smart Battery Sense Temperature is not available\r\n", _logId);
				}
				return true; // we know what to do with it, and we decided to ignore the value
			}

			_tmpFrame.SmartBatterySenseTemperatureMilliCelsius =
				{ millis(), static_cast<int32_t>(data.value) * 10 - 273150 };

			if (_verboseLogging) {
				_msgOut->printf("%s Hex Data: Smart Battery Sense Temperature (0x%04X): %.2f°C\r\n",
						_logId, regLog,
						_tmpFrame.SmartBatterySenseTemperatureMilliCelsius.second / 1000.0);
			}
			return true;
			break;

		case VeDirectHexRegister::NetworkTotalDcInputPower:
			if (data.value == 0xFFFFFFFF) {
				if (_verboseLogging) {
					_msgOut->printf("%s Hex Data: Network total DC power value "
							"indicates non-networked controller\r\n", _logId);
				}
				_tmpFrame.NetworkTotalDcInputPowerMilliWatts = { 0, 0 };
				return true; // we know what to do with it, and we decided to ignore the value
			}

			_tmpFrame.NetworkTotalDcInputPowerMilliWatts =
				{ millis(), data.value * 10 };

			if (_verboseLogging) {
				_msgOut->printf("%s Hex Data: Network Total DC Power (0x%04X): %.2fW\r\n",
						_logId, regLog,
						_tmpFrame.NetworkTotalDcInputPowerMilliWatts.second / 1000.0);
			}
			return true;
			break;

		case VeDirectHexRegister::BatteryAbsorptionVoltage:
			_tmpFrame.BatteryAbsorptionMilliVolt =
				{ millis(), static_cast<uint32_t>(data.value) * 10 };
			if (_verboseLogging) {
				_msgOut->printf("%s Hex Data: MPPT Absorption Voltage (0x%04X): %.2fV\r\n",
						_logId, regLog,
						_tmpFrame.BatteryAbsorptionMilliVolt.second / 1000.0);
			}
			return true;
			break;

		case VeDirectHexRegister::BatteryFloatVoltage:
			_tmpFrame.BatteryFloatMilliVolt =
				{ millis(), static_cast<uint32_t>(data.value) * 10 };

			if (_verboseLogging) {
				_msgOut->printf("%s Hex Data: MPPT Float Voltage (0x%04X): %.2fV\r\n",
						_logId, regLog,
						_tmpFrame.BatteryFloatMilliVolt.second / 1000.0);
			}
			return true;
			break;

#ifdef PROCESS_NETWORK_STATE
		case VeDirectHexRegister::NetworkInfo:
			_tmpFrame.NetworkInfo =
				{ millis(), static_cast<uint8_t>(data.value) };

			if (_verboseLogging) {
				_msgOut->printf("%s Hex Data: Network Info (0x%04X): 0x%X\r\n",
						_logId, regLog, data.value);
			}
			return true;
			break;

		case VeDirectHexRegister::NetworkMode:
			_tmpFrame.NetworkMode =
				{ millis(), static_cast<uint8_t>(data.value) };

			if (_verboseLogging) {
				_msgOut->printf("%s Hex Data: Network Mode (0x%04X): 0x%X\r\n",
						_logId, regLog, data.value);
			}
			return true;
			break;

		case VeDirectHexRegister::NetworkStatus:
			_tmpFrame.NetworkStatus =
				{ millis(), static_cast<uint8_t>(data.value) };

			if (_verboseLogging) {
				_msgOut->printf("%s Hex Data: Network Status (0x%04X): 0x%X\r\n",
						_logId, regLog, data.value);
			}
			return true;
			break;
#endif // PROCESS_NETWORK_STATE

		default:
			return false;
			break;
	}

	return false;
}


/*
 * isHexCommandPossible()
 * return: true = yes we can use Hex-Commands
 */
bool VeDirectMpptController::isHexCommandPossible(void) {
	// Copy from the "VE.Direct Protocol" documentation
	// For firmware version v1.52 and below, when no VE.Direct queries are sent to the device, the
	// charger periodically sends human readable (TEXT) data to the serial port. For firmware
	// versions v1.53 and above, the charger always periodically sends TEXT data to the serial port.
	// --> We just use hex commands for firmware >= 1.53 to keep text messages alive
	return (_canSend && (_tmpFrame.getFwVersionAsInteger() >= 153));
}


/*
 * sendNextHexCommandFromQueue()
 * send one Hex Commands from the Hex Command Queue
 * handles the received hex data from the MPPT
 */
void VeDirectMpptController::sendNextHexCommandFromQueue(void) {
	// It seems some commands get lost if we send to fast the next command.
	// maybe we produce an overflow on the MPPT receive buffer or we have to
	// wait for the MPPT answer before we can send the next command. We only
	// send a new query in VE.Direct idle state and if no query is pending. In
	// case we do not get an answer we send the next query from the queue after
	// a timeout of 500ms. NOTE: _sendTimeout will be set to 0 after receiving
	// an answer, see function hexDataHandler().
	auto millisTime = millis();
	if (isStateIdle() && ((millisTime - _hexQueue[_sendQueueNr]._lastSendTime) > _sendTimeout)) {

		// we do 2 loops, first for high prio commands and second for low prio commands
		bool prio = true;
		for (auto idy = 0; idy < 2; ++idy) {

			// we start searching the queue with the next queue index
			auto idx = _sendQueueNr + 1;
			if (idx >= _hexQueue.size()) { idx = 0; }

			do {
				// we check if it is time to send the command again
				if (((prio && (_hexQueue[idx]._readPeriod == HIGH_PRIO_COMMAND)) ||
					(!prio && (_hexQueue[idx]._readPeriod != HIGH_PRIO_COMMAND))) &&
					(millisTime - _hexQueue[idx]._lastSendTime) > (_hexQueue[idx]._readPeriod * 1000)) {

					sendHexCommand(VeDirectHexCommand::GET, _hexQueue[idx]._hexRegister);
					_hexQueue[idx]._lastSendTime = millisTime;

					// we need this information to check if we get an answer, see hexDataHandler()
					_sendTimeout = 500;
					_sendQueueNr = idx;
					return;
				}

				++idx;
				if (idx == _hexQueue.size()) { idx = 0; }
			} while (idx != _sendQueueNr);

			prio = false; // second loop for low prio commands
		}
	}
}
