#include <Arduino.h>
#include "VeDirectMpptController.h"

void VeDirectMpptController::init(int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging)
{
	VeDirectFrameHandler::init(rx, tx, msgOut, verboseLogging, 1);
	_spData = std::make_shared<veMpptStruct>();
	if (_verboseLogging) { _msgOut->println("Finished init MPPTController"); }
}

bool VeDirectMpptController::isDataValid() const {
	return VeDirectFrameHandler::isDataValid(*_spData);
}

void VeDirectMpptController::textRxEvent(char* name, char* value)
{
	if (VeDirectFrameHandler::textRxEvent("MPPT", name, value, _tmpFrame)) {
		return;
	}

	if (strcmp(name, "LOAD") == 0) {
		if (strcmp(value, "ON") == 0)
			_tmpFrame.LOAD = true;
		else
			_tmpFrame.LOAD = false;
	}
	else if (strcmp(name, "CS") == 0) {
		_tmpFrame.CS = atoi(value);
	}
	else if (strcmp(name, "ERR") == 0) {
		_tmpFrame.ERR = atoi(value);
	}
	else if (strcmp(name, "OR") == 0) {
		_tmpFrame.OR = strtol(value, nullptr, 0);
	}
	else if (strcmp(name, "MPPT") == 0) {
		_tmpFrame.MPPT = atoi(value);
	}
	else if (strcmp(name, "HSDS") == 0) {
		_tmpFrame.HSDS = atoi(value);
	}
	else if (strcmp(name, "VPV") == 0) {
		_tmpFrame.VPV = round(atof(value) / 10.0) / 100.0;
	}
	else if (strcmp(name, "PPV") == 0) {
		_tmpFrame.PPV = atoi(value);
	}
	else if (strcmp(name, "H19") == 0) {
		_tmpFrame.H19 = atof(value) / 100.0;
	}
	else if (strcmp(name, "H20") == 0) {
		_tmpFrame.H20 = atof(value) / 100.0;
	}
	else if (strcmp(name, "H21") == 0) {
		_tmpFrame.H21 = atoi(value);
	}
	else if (strcmp(name, "H22") == 0) {
		_tmpFrame.H22 = atof(value) / 100.0;
	}
	else if (strcmp(name, "H23") == 0) {
		_tmpFrame.H23 = atoi(value);
	}
}

/*
 *  frameValidEvent
 *  This function is called at the end of the received frame.
 */
void VeDirectMpptController::frameValidEvent() {
	_tmpFrame.P = _tmpFrame.V * _tmpFrame.I;

	_tmpFrame.IPV = 0;
	if (_tmpFrame.VPV > 0) {
		_tmpFrame.IPV = _tmpFrame.PPV / _tmpFrame.VPV;
	}

	_tmpFrame.E = 0;
	if ( _tmpFrame.PPV > 0) {
		_efficiency.addNumber(static_cast<double>(_tmpFrame.P * 100) / _tmpFrame.PPV);
		_tmpFrame.E = _efficiency.getAverage();
	}

	_spData = std::make_shared<veMpptStruct>(_tmpFrame);
	_tmpFrame = {};
	_lastUpdate = millis();
}

/*
 * getCsAsString
 * This function returns the state of operations (CS) as readable text.
 */
frozen::string const& VeDirectMpptController::veMpptStruct::getCsAsString() const
{
	static constexpr frozen::map<uint8_t, frozen::string, 9> values = {
		{ 0,   "OFF" },
		{ 2,   "Fault" },
		{ 3,   "Bulk" },
		{ 4,   "Absorbtion" },
		{ 5,   "Float" },
		{ 7,   "Equalize (manual)" },
		{ 245, "Starting-up" },
		{ 247, "Auto equalize / Recondition" },
		{ 252, "External Control" }
	};

	return getAsString(values, CS);
}

/*
 * getMpptAsString
 * This function returns the state of MPPT (MPPT) as readable text.
 */
frozen::string const& VeDirectMpptController::veMpptStruct::getMpptAsString() const
{
	static constexpr frozen::map<uint8_t, frozen::string, 3> values = {
		{ 0, "OFF" },
		{ 1, "Voltage or current limited" },
		{ 2, "MPP Tracker active" }
	};

	return getAsString(values, MPPT);
}

/*
 * getErrAsString
 * This function returns error state (ERR) as readable text.
 */
frozen::string const& VeDirectMpptController::veMpptStruct::getErrAsString() const
{
	static constexpr frozen::map<uint8_t, frozen::string, 20> values = {
		{ 0,   "No error" },
		{ 2,   "Battery voltage too high" },
		{ 17,  "Charger temperature too high" },
		{ 18,  "Charger over current" },
		{ 19,  "Charger current reversed" },
		{ 20,  "Bulk time limit exceeded" },
		{ 21,  "Current sensor issue(sensor bias/sensor broken)" },
		{ 26,  "Terminals overheated" },
		{ 28,  "Converter issue (dual converter models only)" },
		{ 33,  "Input voltage too high (solar panel)" },
		{ 34,  "Input current too high (solar panel)" },
		{ 38,  "Input shutdown (due to excessive battery voltage)" },
		{ 39,  "Input shutdown (due to current flow during off mode)" },
		{ 40,  "Input" },
		{ 65,  "Lost communication with one of devices" },
		{ 67,  "Synchronisedcharging device configuration issue" },
		{ 68,  "BMS connection lost" },
		{ 116, "Factory calibration data lost" },
		{ 117, "Invalid/incompatible firmware" },
		{ 118, "User settings invalid" }
	};

	return getAsString(values, ERR);
}

/*
 * getOrAsString
 * This function returns the off reason (OR) as readable text.
 */
frozen::string const& VeDirectMpptController::veMpptStruct::getOrAsString() const
{
	static constexpr frozen::map<uint32_t, frozen::string, 10> values = {
		{ 0x00000000, "Not off" },
		{ 0x00000001, "No input power" },
		{ 0x00000002, "Switched off (power switch)" },
		{ 0x00000004, "Switched off (device moderegister)" },
		{ 0x00000008, "Remote input" },
		{ 0x00000010, "Protection active" },
		{ 0x00000020, "Paygo" },
		{ 0x00000040, "BMS" },
		{ 0x00000080, "Engine shutdown detection" },
		{ 0x00000100, "Analysing input voltage" }
	};

	return getAsString(values, OR);
}
