#include <Arduino.h>
#include "VeDirectMpptController.h"

void VeDirectMpptController::init(int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging, uint16_t hwSerialPort)
{
	VeDirectFrameHandler::init("MPPT", rx, tx, msgOut, verboseLogging, hwSerialPort);
	_spData = std::make_shared<veMpptStruct>();
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
		_efficiency.addNumber(static_cast<double>(_tmpFrame.P * 100) / _tmpFrame.PPV);
		_tmpFrame.E = _efficiency.getAverage();
	}

	_spData = std::make_shared<veMpptStruct>(_tmpFrame);
}
