#include <Arduino.h>
#include "VeDirectShuntController.h"

VeDirectShuntController VeDirectShunt;

VeDirectShuntController::VeDirectShuntController()
{
}

void VeDirectShuntController::init(int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging)
{
	VeDirectFrameHandler::init("SmartShunt", rx, tx, msgOut, verboseLogging, 2);
}

void VeDirectShuntController::textRxEvent(char* name, char* value)
{
	if (VeDirectFrameHandler::textRxEvent(name, value, _tmpFrame)) {
		return;
	}

	if (strcmp(name, "T") == 0) {
		_tmpFrame.T = atoi(value);
		_tmpFrame.tempPresent = true;
	}
	else if (strcmp(name, "P") == 0) {
		_tmpFrame.P = atoi(value);
	}
	else if (strcmp(name, "CE") == 0) {
		_tmpFrame.CE = atoi(value);
	}
	else if (strcmp(name, "SOC") == 0) {
		_tmpFrame.SOC = atoi(value);
	}
	else if (strcmp(name, "TTG") == 0) {
		_tmpFrame.TTG = atoi(value);
	}
	else if (strcmp(name, "ALARM") == 0) {
		_tmpFrame.ALARM = (strcmp(value, "ON") == 0);
	}
	else if (strcmp(name, "H1") == 0) {
		_tmpFrame.H1 = atoi(value);
	}
	else if (strcmp(name, "H2") == 0) {
		_tmpFrame.H2 = atoi(value);
	}
	else if (strcmp(name, "H3") == 0) {
		_tmpFrame.H3 = atoi(value);
	}
	else if (strcmp(name, "H4") == 0) {
		_tmpFrame.H4 = atoi(value);
	}
	else if (strcmp(name, "H5") == 0) {
		_tmpFrame.H5 = atoi(value);
	}
	else if (strcmp(name, "H6") == 0) {
		_tmpFrame.H6 = atoi(value);
	}
	else if (strcmp(name, "H7") == 0) {
		_tmpFrame.H7 = atoi(value);
	}
	else if (strcmp(name, "H8") == 0) {
		_tmpFrame.H8 = atoi(value);
	}
	else if (strcmp(name, "H9") == 0) {
		_tmpFrame.H9 = atoi(value);
	}
	else if (strcmp(name, "H10") == 0) {
		_tmpFrame.H10 = atoi(value);
	}
	else if (strcmp(name, "H11") == 0) {
		_tmpFrame.H11 = atoi(value);
	}
	else if (strcmp(name, "H12") == 0) {
		_tmpFrame.H12 = atoi(value);
	}
	else if (strcmp(name, "H13") == 0) {
		_tmpFrame.H13 = atoi(value);
	}
	else if (strcmp(name, "H14") == 0) {
		_tmpFrame.H14 = atoi(value);
	}
	else if (strcmp(name, "H15") == 0) {
		_tmpFrame.H15 = atoi(value);
	}
	else if (strcmp(name, "H16") == 0) {
		_tmpFrame.H16 = atoi(value);
	}
	else if (strcmp(name, "H17") == 0) {
		_tmpFrame.H17 = atoi(value);
	}
	else if (strcmp(name, "H18") == 0) {
		_tmpFrame.H18 = atoi(value);
	}
}

/*
 *  frameValidEvent
 *  This function is called at the end of the received frame.
 */
void VeDirectShuntController::frameValidEvent() {
	// other than in the MPPT controller, the SmartShunt seems to split all data
	// into two seperate messagesas. Thus we update veFrame only every second message
	// after a value for PID has been received
	if (_tmpFrame.PID == 0) { return; }

	veFrame = _tmpFrame;
	_tmpFrame = {};
	_lastUpdate = millis();
}
