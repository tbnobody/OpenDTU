#include <Arduino.h>
#include "VeDirectShuntController.h"

VeDirectShuntController VeDirectShunt;

void VeDirectShuntController::init(int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging)
{
	VeDirectFrameHandler::init("SmartShunt", rx, tx, msgOut, verboseLogging, 2);
}

bool VeDirectShuntController::processTextDataDerived(std::string const& name, std::string const& value)
{
	if (name == "T") {
		_tmpFrame.T = atoi(value.c_str());
		_tmpFrame.tempPresent = true;
		return true;
	}
	if (name == "P") {
		_tmpFrame.P = atoi(value.c_str());
		return true;
	}
	if (name == "CE") {
		_tmpFrame.CE = atoi(value.c_str());
		return true;
	}
	if (name == "SOC") {
		_tmpFrame.SOC = atoi(value.c_str());
		return true;
	}
	if (name == "TTG") {
		_tmpFrame.TTG = atoi(value.c_str());
		return true;
	}
	if (name == "ALARM") {
		_tmpFrame.ALARM = (value == "ON");
		return true;
	}
	if (name == "AR") {
		_tmpFrame.alarmReason_AR = atoi(value.c_str());
		return true;
	}
	if (name == "H1") {
		_tmpFrame.H1 = atoi(value.c_str());
		return true;
	}
	if (name == "H2") {
		_tmpFrame.H2 = atoi(value.c_str());
		return true;
	}
	if (name == "H3") {
		_tmpFrame.H3 = atoi(value.c_str());
		return true;
	}
	if (name == "H4") {
		_tmpFrame.H4 = atoi(value.c_str());
		return true;
	}
	if (name == "H5") {
		_tmpFrame.H5 = atoi(value.c_str());
		return true;
	}
	if (name == "H6") {
		_tmpFrame.H6 = atoi(value.c_str());
		return true;
	}
	if (name == "H7") {
		_tmpFrame.H7 = atoi(value.c_str());
		return true;
	}
	if (name == "H8") {
		_tmpFrame.H8 = atoi(value.c_str());
		return true;
	}
	if (name == "H9") {
		_tmpFrame.H9 = atoi(value.c_str());
		return true;
	}
	if (name == "H10") {
		_tmpFrame.H10 = atoi(value.c_str());
		return true;
	}
	if (name == "H11") {
		_tmpFrame.H11 = atoi(value.c_str());
		return true;
	}
	if (name == "H12") {
		_tmpFrame.H12 = atoi(value.c_str());
		return true;
	}
	if (name == "H13") {
		_tmpFrame.H13 = atoi(value.c_str());
		return true;
	}
	if (name == "H14") {
		_tmpFrame.H14 = atoi(value.c_str());
		return true;
	}
	if (name == "H15") {
		_tmpFrame.H15 = atoi(value.c_str());
		return true;
	}
	if (name == "H16") {
		_tmpFrame.H16 = atoi(value.c_str());
		return true;
	}
	if (name == "H17") {
		_tmpFrame.H17 = atoi(value.c_str());
		return true;
	}
	if (name == "H18") {
		_tmpFrame.H18 = atoi(value.c_str());
		return true;
	}
	if (name == "BMV") {
		// This field contains a textual description of the BMV model,
		// for example 602S or 702. It is deprecated, refer to the field PID instead.
		return true;
	}
	if (name == "MON") {
		_tmpFrame.dcMonitorMode_MON = static_cast<int8_t>(atoi(value.c_str()));
		return true;
	}
	return false;
}
