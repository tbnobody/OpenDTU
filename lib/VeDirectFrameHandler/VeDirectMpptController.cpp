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
	if (name == "LOAD") {
		_tmpFrame.LOAD = (value == "ON");
		return true;
	}
	if (name == "CS") {
		_tmpFrame.CS = atoi(value.c_str());
		return true;
	}
	if (name == "ERR") {
		_tmpFrame.ERR = atoi(value.c_str());
		return true;
	}
	if (name == "OR") {
		_tmpFrame.OR = strtol(value.c_str(), nullptr, 0);
		return true;
	}
	if (name == "MPPT") {
		_tmpFrame.MPPT = atoi(value.c_str());
		return true;
	}
	if (name == "HSDS") {
		_tmpFrame.HSDS = atoi(value.c_str());
		return true;
	}
	if (name == "VPV") {
		_tmpFrame.VPV = round(atof(value.c_str()) / 10.0) / 100.0;
		return true;
	}
	if (name == "PPV") {
		_tmpFrame.PPV = atoi(value.c_str());
		return true;
	}
	if (name == "H19") {
		_tmpFrame.H19 = atof(value.c_str()) / 100.0;
		return true;
	}
	if (name == "H20") {
		_tmpFrame.H20 = atof(value.c_str()) / 100.0;
		return true;
	}
	if (name == "H21") {
		_tmpFrame.H21 = atoi(value.c_str());
		return true;
	}
	if (name == "H22") {
		_tmpFrame.H22 = atof(value.c_str()) / 100.0;
		return true;
	}
	if (name == "H23") {
		_tmpFrame.H23 = atoi(value.c_str());
		return true;
	}

	return false;
}

/*
 *  frameValidEvent
 *  This function is called at the end of the received frame.
 */
void VeDirectMpptController::frameValidEvent() {
	_tmpFrame.P = _tmpFrame.V * _tmpFrame.I;

	if (_tmpFrame.VPV > 0) {
		_tmpFrame.IPV = _tmpFrame.PPV / _tmpFrame.VPV;
	}

	if (_tmpFrame.PPV > 0) {
		_efficiency.addNumber(static_cast<float>(_tmpFrame.P * 100) / _tmpFrame.PPV);
		_tmpFrame.E = _efficiency.getAverage();
	}

	if (!_canSend) { return; }

	// Copy from the "VE.Direct Protocol" documentation
	// For firmware version v1.52 and below, when no VE.Direct queries are sent to the device, the
	// charger periodically sends human readable (TEXT) data to the serial port. For firmware
	// versions v1.53 and above, the charger always periodically sends TEXT data to the serial port.
	// --> We just use hex commandes for firmware >= 1.53 to keep text messages alive
	if (atoi(_tmpFrame.FW) < 153) { return; }

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
