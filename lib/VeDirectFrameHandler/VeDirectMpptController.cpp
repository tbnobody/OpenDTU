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

void VeDirectMpptController::init(int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging, uint16_t hwSerialPort)
{
	VeDirectFrameHandler::init("MPPT", rx, tx, msgOut, verboseLogging, hwSerialPort);
}

bool VeDirectMpptController::processTextDataDerived(std::string const& name, std::string const& value)
{
	if (name == "IL") {
		_tmpFrame.loadCurrent_IL_mA = atol(value.c_str());
		return true;
	}
	if (name == "LOAD") {
		_tmpFrame.loadOutputState_LOAD = (value == "ON");
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
	_tmpFrame.batteryOutputPower_W = static_cast<int16_t>(_tmpFrame.batteryVoltage_V_mV * _tmpFrame.batteryCurrent_I_mA / 1000000);

	if ((_tmpFrame.panelVoltage_VPV_mV > 0) && (_tmpFrame.panelPower_PPV_W >= 1)) {
		_tmpFrame.panelCurrent_mA = static_cast<uint32_t>(_tmpFrame.panelPower_PPV_W * 1000000) / _tmpFrame.panelVoltage_VPV_mV;
	}

	if (_tmpFrame.panelPower_PPV_W > 0) {
		_efficiency.addNumber(static_cast<float>(_tmpFrame.batteryOutputPower_W * 100) / _tmpFrame.panelPower_PPV_W);
		_tmpFrame.mpptEfficiency_Percent = _efficiency.getAverage();
	}

	if (!_canSend) { return; }

	// Copy from the "VE.Direct Protocol" documentation
	// For firmware version v1.52 and below, when no VE.Direct queries are sent to the device, the
	// charger periodically sends human readable (TEXT) data to the serial port. For firmware
	// versions v1.53 and above, the charger always periodically sends TEXT data to the serial port.
	// --> We just use hex commandes for firmware >= 1.53 to keep text messages alive
	if (atoi(_tmpFrame.firmwareNr_FW) < 153) { return; }

	using Command = VeDirectHexCommand;
	using Register = VeDirectHexRegister;

	sendHexCommand(Command::GET, Register::ChargeControllerTemperature);
	sendHexCommand(Command::GET, Register::SmartBatterySenseTemperature);
	sendHexCommand(Command::GET, Register::NetworkTotalDcInputPower);

#ifdef PROCESS_NETWORK_STATE
	sendHexCommand(Command::GET, Register::NetworkInfo);
	sendHexCommand(Command::GET, Register::NetworkMode);
	sendHexCommand(Command::GET, Register::NetworkStatus);
#endif // PROCESS_NETWORK_STATE
}


void VeDirectMpptController::loop()
{
	VeDirectFrameHandler::loop();

	auto resetTimestamp = [this](auto& pair) {
		if (pair.first > 0 && (millis() - pair.first) > (10 * 1000)) {
			pair.first = 0;
		}
	};

	resetTimestamp(_tmpFrame.MpptTemperatureMilliCelsius);
	resetTimestamp(_tmpFrame.SmartBatterySenseTemperatureMilliCelsius);
	resetTimestamp(_tmpFrame.NetworkTotalDcInputPowerMilliWatts);

#ifdef PROCESS_NETWORK_STATE
	resetTimestamp(_tmpFrame.NetworkInfo);
	resetTimestamp(_tmpFrame.NetworkMode);
	resetTimestamp(_tmpFrame.NetworkStatus);
#endif // PROCESS_NETWORK_STATE
}


/*
 * hexDataHandler()
 * analyse the content of VE.Direct hex messages
 * Handels the received hex data from the MPPT
 */
bool VeDirectMpptController::hexDataHandler(VeDirectHexData const &data) {
	if (data.rsp != VeDirectHexResponse::GET &&
			data.rsp != VeDirectHexResponse::ASYNC) { return false; }

	auto regLog = static_cast<uint16_t>(data.addr);

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
				{ millis(), static_cast<int32_t>(data.value) * 10 - 272150 };

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
