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


// support for debugging, 0=without extended logging, 1=with extended logging
constexpr int MODUL_DEBUG = 0;


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
}


/*
// loop()
// send hex commands to MPPT every 5 seconds
*/
void VeDirectMpptController::loop()
{
	VeDirectFrameHandler::loop();
	
	// Copy from the "VE.Direct Protocol" documentation
	// For firmware version v1.52 and below, when no VE.Direct queries are sent to the device, the
	// charger periodically sends human readable (TEXT) data to the serial port. For firmware
	// versions v1.53 and above, the charger always periodically sends TEXT data to the serial port.
	// --> We just use hex commandes for firmware >= 1.53 to keep text messages alive
	if (atoi(_tmpFrame.FW) >= 153 ) {
		if ((millis() - _lastPingTime) > 5000) {

			sendHexCommand(GET, 0x2027);	// MPPT total DC input power
			sendHexCommand(GET, 0xEDDB);	// MPPT internal temperature
			sendHexCommand(GET, 0xEDEC);	// "Smart Battery Sense" temperature
			sendHexCommand(GET, 0x200F);	// Network info
			_lastPingTime = millis();
		}
	}
}


/*
 * hexDataHandler()
 * analyse the content of VE.Direct hex messages
 * Handels the received hex data from the MPPT
 */
void VeDirectMpptController::hexDataHandler(VeDirectHexData const &data) {
    bool state = false;

	switch (data.rsp) {
	case R_GET:
	case R_ASYNC:    

		// check if MPPT internal temperature is available
		if(data.id == 0xEDDB) {
			_ExData.T = static_cast<int32_t>(data.value) * 10;	// conversion from unit [0.01°C] to unit [m°C]
			_ExData.Tts = millis();
			state = true;
			
			if constexpr(MODUL_DEBUG == 1)
				_msgOut->printf("[VE.Direct] debug: hexDataHandler(), MTTP Temperature: %.2f°C\r\n", _ExData.T/1000.0);
		}

		// check if temperature from "Smart Battery Sense" is available
		if(data.id == 0xEDEC) {
			_ExData.TSBS = static_cast<int32_t>(data.value) * 10 - 272150;  // conversion from unit [0.01K] to unit [m°C]
			_ExData.TSBSts = millis();
			state = true;
			
			if constexpr(MODUL_DEBUG == 1)
				_msgOut->printf("[VE.Direct] debug: hexDataHandler(), Battery Temperature: %.2f°C\r\n", _ExData.TSBS/1000.0);
		}

		// check if "Total DC power" is available
		if(data.id == 0x2027) {
			_ExData.TDCP = data.value * 10;		// conversion from unit [0.01W] to unit [mW]
			_ExData.TDCPts = millis();
			state = true;

			if constexpr(MODUL_DEBUG == 1)
				_msgOut->printf("[VE.Direct] debug: hexDataHandler(), Total Power: %.2fW\r\n", _ExData.TDCP/1000.0);
		}

		// check if connected MPPT is charge instance master
		// Hint: not used right now but maybe necessary for future extensions
		if(data.id == 0x200F) {
			_veMaster = ((data.value & 0x0F) == 0x02) ? true : false;
			state = true;

			if constexpr(MODUL_DEBUG == 1)
				_msgOut->printf("[VE.Direct] debug: hexDataHandler(), Networkmode: 0x%X\r\n", data.value);
		}
		break;
	default:
		break;	
	}

	if constexpr(MODUL_DEBUG == 1)
		_msgOut->printf("[VE.Direct] debug: hexDataHandler(): rsp: %i, id: 0x%04X, value: %i[0x%08X], text: %s\r\n",
				data.rsp, data.id, data.value, data.value, data.text);

	if (_verboseLogging && state) 
		_msgOut->printf("[VE.Direct] MPPT hex message: rsp: %i, id: 0x%04X, value: %i[0x%08X], text: %s\r\n",
			data.rsp, data.id, data.value, data.value, data.text);
}   
