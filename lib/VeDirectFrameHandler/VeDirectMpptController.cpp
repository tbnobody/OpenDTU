#include <Arduino.h>
#include <map>
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
String VeDirectMpptController::veMpptStruct::getCsAsString() const
{
	static const std::map<uint8_t, String> values = {
		{ 0,   F("OFF") },
		{ 2,   F("Fault") },
		{ 3,   F("Bulk") },
		{ 4,   F("Absorbtion") },
		{ 5,   F("Float") },
		{ 7,   F("Equalize (manual)") },
		{ 245, F("Starting-up") },
		{ 247, F("Auto equalize / Recondition") },
		{ 252, F("External Control") }
	};

	return getAsString(values, CS);
}

/*
 * getMpptAsString
 * This function returns the state of MPPT (MPPT) as readable text.
 */
String VeDirectMpptController::veMpptStruct::getMpptAsString() const
{
	static const std::map<uint8_t, String> values = {
		{ 0, F("OFF") },
		{ 1, F("Voltage or current limited") },
		{ 2, F("MPP Tracker active") }
	};

	return getAsString(values, MPPT);
}

/*
 * getErrAsString
 * This function returns error state (ERR) as readable text.
 */
String VeDirectMpptController::veMpptStruct::getErrAsString() const
{
	static const std::map<uint8_t, String> values = {
		{ 0,   F("No error") },
		{ 2,   F("Battery voltage too high") },
		{ 17,  F("Charger temperature too high") },
		{ 18,  F("Charger over current") },
		{ 19,  F("Charger current reversed") },
		{ 20,  F("Bulk time limit exceeded") },
		{ 21,  F("Current sensor issue(sensor bias/sensor broken)") },
		{ 26,  F("Terminals overheated") },
		{ 28,  F("Converter issue (dual converter models only)") },
		{ 33,  F("Input voltage too high (solar panel)") },
		{ 34,  F("Input current too high (solar panel)") },
		{ 38,  F("Input shutdown (due to excessive battery voltage)") },
		{ 39,  F("Input shutdown (due to current flow during off mode)") },
		{ 40,  F("Input") },
		{ 65,  F("Lost communication with one of devices") },
		{ 67,  F("Synchronisedcharging device configuration issue") },
		{ 68,  F("BMS connection lost") },
		{ 116, F("Factory calibration data lost") },
		{ 117, F("Invalid/incompatible firmware") },
		{ 118, F("User settings invalid") }
	};

	return getAsString(values, ERR);
}

/*
 * getOrAsString
 * This function returns the off reason (OR) as readable text.
 */
String VeDirectMpptController::veMpptStruct::getOrAsString() const
{
	static const std::map<uint32_t, String> values = {
		{ 0x00000000, F("Not off") },
		{ 0x00000001, F("No input power") },
		{ 0x00000002, F("Switched off (power switch)") },
		{ 0x00000004, F("Switched off (device moderegister)") },
		{ 0x00000008, F("Remote input") },
		{ 0x00000010, F("Protection active") },
		{ 0x00000020, F("Paygo") },
		{ 0x00000040, F("BMS") },
		{ 0x00000080, F("Engine shutdown detection") },
		{ 0x00000100, F("Analysing input voltage") }
	};

	return getAsString(values, OR);
}
