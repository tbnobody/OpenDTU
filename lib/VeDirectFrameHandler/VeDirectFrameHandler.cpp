/* framehandler.cpp
 *
 * Arduino library to read from Victron devices using VE.Direct protocol.
 * Derived from Victron framehandler reference implementation.
 * 
 * The MIT License
 * 
 * Copyright (c) 2019 Victron Energy BV
 * Portions Copyright (C) 2020 Chris Terwilliger
 * https://github.com/cterwilliger/VeDirectFrameHandler
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *  
 * 2020.05.05 - 0.2 - initial release
 * 2020.06.21 - 0.2 - add MIT license, no code changes
 * 2020.08.20 - 0.3 - corrected #include reference
 * 
 */
 
#include <Arduino.h>
#include "VeDirectFrameHandler.h"

// The name of the record that contains the checksum.
static constexpr char checksumTagName[] = "CHECKSUM";

// state machine
enum States {
	IDLE = 1,
	RECORD_BEGIN = 2,
	RECORD_NAME = 3,
	RECORD_VALUE = 4,
	CHECKSUM = 5,
	RECORD_HEX = 6
};

HardwareSerial VedirectSerial(1);

VeDirectFrameHandler VeDirect;

class Silent : public Print {
    public:
        size_t write(uint8_t c) final { return 0; }
};

static Silent MessageOutputDummy;

VeDirectFrameHandler::VeDirectFrameHandler() :
	//mStop(false),	// don't know what Victron uses this for, not using
	_msgOut(&MessageOutputDummy),
	_state(IDLE),
	_checksum(0),
	_textPointer(0),
	_hexSize(0),
	_name(""),
	_value(""),
	_tmpFrame(),
	_debugIn(0),
	_lastByteMillis(0),
	_lastUpdate(0)
{
}

void VeDirectFrameHandler::setVerboseLogging(bool verboseLogging)
{
	_verboseLogging = verboseLogging;
}

void VeDirectFrameHandler::init(int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging)
{
	VedirectSerial.begin(19200, SERIAL_8N1, rx, tx);
	VedirectSerial.flush();
	_msgOut = msgOut;
	setVerboseLogging(verboseLogging);
}

void VeDirectFrameHandler::dumpDebugBuffer() {
	_msgOut->printf("[VE.Direct] serial input (%d Bytes):", _debugIn);
	for (int i = 0; i < _debugIn; ++i) {
		if (i % 16 == 0) {
			_msgOut->printf("\r\n[VE.Direct]");
		}
		_msgOut->printf(" %02x", _debugBuffer[i]);
	}
	_msgOut->println("");
	_debugIn = 0;
}

void VeDirectFrameHandler::loop()
{
	while ( VedirectSerial.available()) {
		rxData(VedirectSerial.read());
		_lastByteMillis = millis();
	}

	// there will never be a large gap between two bytes of the same frame.
	// if such a large gap is observed, reset the state machine so it tries
	// to decode a new frame once more data arrives.
	if (IDLE != _state && _lastByteMillis + 500 < millis()) {
		_msgOut->printf("[VE.Direct] Resetting state machine (was %d) after timeout\r\n", _state);
		if (_verboseLogging) { dumpDebugBuffer(); }
		_checksum = 0;
		_state = IDLE;
		_tmpFrame = { };
	}
}

/*
 *	rxData
 *  This function is called by loop() which passes a byte of serial data
 *  Based on Victron's example code. But using String and Map instead of pointer and arrays
 */
void VeDirectFrameHandler::rxData(uint8_t inbyte)
{
	_debugBuffer[_debugIn] = inbyte;
	_debugIn = (_debugIn + 1) % _debugBuffer.size();
	if (0 == _debugIn) {
		_msgOut->println("[VE.Direct] ERROR: debug buffer overrun!");
	}

	//if (mStop) return;
	if ( (inbyte == ':') && (_state != CHECKSUM) ) {
		_prevState = _state; //hex frame can interrupt TEXT
		_state = RECORD_HEX; 
		_hexSize = 0;
	}
	if (_state != RECORD_HEX) {
		_checksum += inbyte;
	}
	inbyte = toupper(inbyte);

	switch(_state) {
	case IDLE:
		/* wait for \n of the start of an record */
		switch(inbyte) {
		case '\n':
			_state = RECORD_BEGIN;
			break;
		case '\r': /* Skip */
		default:
			break;
		}
		break;
	case RECORD_BEGIN:
		_textPointer = _name;
		*_textPointer++ = inbyte;
		_state = RECORD_NAME;
		break;
	case RECORD_NAME:
		// The record name is being received, terminated by a \t
		switch(inbyte) {
		case '\t':
			// the Checksum record indicates a EOR
			if ( _textPointer < (_name + sizeof(_name)) ) {
				*_textPointer = 0; /* Zero terminate */
				if (strcmp(_name, checksumTagName) == 0) {
					_state = CHECKSUM;
					break;
				}
			}
			_textPointer = _value; /* Reset value pointer */
			_state = RECORD_VALUE;
			break;
		case '#': /* Ignore # from serial number*/
			break;
		default:
			// add byte to name, but do no overflow
			if ( _textPointer < (_name + sizeof(_name)) )
				*_textPointer++ = inbyte;
			break;
		}
		break;
	case RECORD_VALUE:
		// The record value is being received.  The \r indicates a new record.
		switch(inbyte) {
		case '\n':
			if ( _textPointer < (_value + sizeof(_value)) ) {
				*_textPointer = 0; // make zero ended
				textRxEvent(_name, _value);
			}
			_state = RECORD_BEGIN;
			break;
		case '\r': /* Skip */
			break;
		default:
			// add byte to value, but do no overflow
			if ( _textPointer < (_value + sizeof(_value)) )
				*_textPointer++ = inbyte;
			break;
		}
		break;
	case CHECKSUM:
	{
		bool valid = _checksum == 0;
		if (!valid) {
			_msgOut->printf("[VE.Direct] checksum 0x%02x != 0, invalid frame\r\n", _checksum);
		}
		if (_verboseLogging) { dumpDebugBuffer(); }
		_checksum = 0;
		_state = IDLE;
		frameEndEvent(valid);
		break;
	}
	case RECORD_HEX:
		_state = hexRxEvent(inbyte);
		break;
	}
}

/*
 * textRxEvent
 * This function is called every time a new name/value is successfully parsed.  It writes the values to the temporary buffer.
 */
void VeDirectFrameHandler::textRxEvent(char * name, char * value) {
	if (strcmp(name, "PID") == 0) {
		_tmpFrame.PID = strtol(value, nullptr, 0);
	}
	else if (strcmp(name, "SER") == 0) {
		strcpy(_tmpFrame.SER, value);
	}
	else if (strcmp(name, "FW") == 0) {
		strcpy(_tmpFrame.FW, value);
	}
	else if (strcmp(name, "LOAD") == 0) {
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
	else if (strcmp(name, "V") == 0) {
		_tmpFrame.V = round(atof(value) / 10.0) / 100.0;
	}
	else if (strcmp(name, "I") == 0) {
		_tmpFrame.I = round(atof(value) / 10.0) / 100.0;
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
 *	frameEndEvent
 *  This function is called at the end of the received frame.  If the checksum is valid, the temp buffer is read line by line.
 *  If the name exists in the public buffer, the new value is copied to the public buffer.	If not, a new name/value entry
 *  is created in the public buffer.
 */
void VeDirectFrameHandler::frameEndEvent(bool valid) {
	if ( valid ) {
		_tmpFrame.P = _tmpFrame.V * _tmpFrame.I;

		_tmpFrame.IPV = 0;
		if ( _tmpFrame.VPV > 0) {
			_tmpFrame.IPV = _tmpFrame.PPV / _tmpFrame.VPV;
		}

		_tmpFrame.E = 0;
		if ( _tmpFrame.PPV > 0) {
			_efficiency.addNumber(static_cast<double>(_tmpFrame.P * 100) / _tmpFrame.PPV);
			_tmpFrame.E = _efficiency.getAverage();
		}

		veFrame = _tmpFrame;
		_lastUpdate = millis();
	}
	_tmpFrame = {};
}

/*
 *	hexRxEvent
 *  This function records hex answers or async messages	
 */
int VeDirectFrameHandler::hexRxEvent(uint8_t inbyte) {
	int ret=RECORD_HEX; // default - continue recording until end of frame

	switch (inbyte) {
	case '\n':
		// restore previous state
		ret=_prevState;
		break;
		
	default:
		_hexSize++;
		if (_hexSize>=VE_MAX_HEX_LEN) { // oops -buffer overflow - something went wrong, we abort
			_msgOut->println("[VE.Direct] hexRx buffer overflow - aborting read");
			_hexSize=0;
			ret=IDLE;
		}
	}
	
	return ret;
}

bool VeDirectFrameHandler::isDataValid() {
	if (_lastUpdate == 0) {
		return false;
	}
	if (strlen(veFrame.SER) == 0) {
		return false;
	}
	return true;
}

unsigned long VeDirectFrameHandler::getLastUpdate()
{
    return _lastUpdate;
}

/*
 * getPidAsString
 * This function returns the product id (PID) as readable text.
 */
String VeDirectFrameHandler::getPidAsString(uint16_t pid)
{
	String strPID ="";

	switch(pid) {
		case 0x0300:
			strPID =  "BlueSolar MPPT 70|15";
			break;
		case 0xA040:
			strPID =  "BlueSolar MPPT 75|50";
			break;
		case 0xA041:
			strPID =  "BlueSolar MPPT 150|35";
			break;
		case 0xA042:
			strPID =  "BlueSolar MPPT 75|15";
			break;
		case 0xA043:
			strPID =  "BlueSolar MPPT 100|15";
			break;
		case 0xA044:
			strPID =  "BlueSolar MPPT 100|30";
			break;
		case 0xA045:
			strPID =  "BlueSolar MPPT 100|50";
			break;
		case 0xA046:
			strPID =  "BlueSolar MPPT 100|70";
			break;
		case 0xA047:
			strPID =  "BlueSolar MPPT 150|100";
			break;
		case 0xA049:
			strPID =  "BlueSolar MPPT 100|50 rev2";
			break;
		case 0xA04A:
			strPID =  "BlueSolar MPPT 100|30 rev2";
			break;
		case 0xA04B:
			strPID =  "BlueSolar MPPT 150|35 rev2";
			break;
		case 0XA04C:
			strPID =  "BlueSolar MPPT 75|10";
			break;
		case 0XA04D:
			strPID =  "BlueSolar MPPT 150|45";
			break;
		case 0XA04E:
			strPID =  "BlueSolar MPPT 150|60";
			break;
		case 0XA04F:
			strPID =  "BlueSolar MPPT 150|85";
			break;
		case 0XA050:
			strPID =  "SmartSolar MPPT 250|100";
			break;
		case 0XA051:
			strPID =  "SmartSolar MPPT 150|100";
			break;
		case 0XA052:
			strPID =  "SmartSolar MPPT 150|85";
			break;
		case 0XA053:
			strPID =  "SmartSolar MPPT 75|15";
			break;
		case 0XA054:
			strPID =  "SmartSolar MPPT 75|10";
			break;
		case 0XA055:
			strPID =  "SmartSolar MPPT 100|15";
			break;
		case 0XA056:
			strPID =  "SmartSolar MPPT 100|30";
			break;
		case 0XA057:
			strPID =  "SmartSolar MPPT 100|50";
			break;
		case 0XA058:
			strPID =  "SmartSolar MPPT 150|35";
			break;
		case 0XA059:
			strPID =  "SmartSolar MPPT 150|10 rev2";
			break;
		case 0XA05A:
			strPID =  "SmartSolar MPPT 150|85 rev2";
			break;
		case 0XA05B:
			strPID =  "SmartSolar MPPT 250|70";
			break;
		case 0XA05C:
			strPID =  "SmartSolar MPPT 250|85";
			break;
		case 0XA05D:
			strPID =  "SmartSolar MPPT 250|60";
			break;
		case 0XA05E:
			strPID =  "SmartSolar MPPT 250|45";
			break;
		case 0XA05F:
			strPID =  "SmartSolar MPPT 100|20";
			break;
		case 0XA060:
			strPID =  "SmartSolar MPPT 100|20 48V";
			break;
		case 0XA061:
			strPID =  "SmartSolar MPPT 150|45";
			break;
		case 0XA062:
			strPID =  "SmartSolar MPPT 150|60";
			break;
		case 0XA063:
			strPID =  "SmartSolar MPPT 150|70";
			break;
		case 0XA064:
			strPID =  "SmartSolar MPPT 250|85 rev2";
			break;
		case 0XA065:
			strPID =  "SmartSolar MPPT 250|100 rev2";
			break;
		case 0XA066:
			strPID =  "BlueSolar MPPT 100|20";
			break;
		case 0XA067:
			strPID =  "BlueSolar MPPT 100|20 48V";
			break;
		case 0XA068:
			strPID =  "SmartSolar MPPT 250|60 rev2";
			break;
		case 0XA069:
			strPID =  "SmartSolar MPPT 250|70 rev2";
			break;
		case 0XA06A:
			strPID =  "SmartSolar MPPT 150|45 rev2";
			break;
		case 0XA06B:
			strPID =  "SmartSolar MPPT 150|60 rev2";
			break;
		case 0XA06C:
			strPID =  "SmartSolar MPPT 150|70 rev2";
			break;
		case 0XA06D:
			strPID =  "SmartSolar MPPT 150|85 rev3";
			break;
		case 0XA06E:
			strPID =  "SmartSolar MPPT 150|100 rev3";
			break;
		case 0XA06F:
			strPID =  "BlueSolar MPPT 150|45 rev2";
			break;
		case 0XA070:
			strPID =  "BlueSolar MPPT 150|60 rev2";
			break;
		case 0XA071:
			strPID =  "BlueSolar MPPT 150|70 rev2";
			break;
		case 0XA102:
			strPID =  "SmartSolar MPPT VE.Can 150|70";
			break;
		case 0XA103:
			strPID =  "SmartSolar MPPT VE.Can 150|45";
			break;
		case 0XA104:
			strPID =  "SmartSolar MPPT VE.Can 150|60";
			break;
		case 0XA105:
			strPID =  "SmartSolar MPPT VE.Can 150|85";
			break;
		case 0XA106:
			strPID =  "SmartSolar MPPT VE.Can 150|100";
			break;
		case 0XA107:
			strPID =  "SmartSolar MPPT VE.Can 250|45";
			break;
		case 0XA108:
			strPID =  "SmartSolar MPPT VE.Can 250|60";
			break;
		case 0XA109:
			strPID =  "SmartSolar MPPT VE.Can 250|80";
			break;
		case 0XA10A:
			strPID =  "SmartSolar MPPT VE.Can 250|85";
			break;
		case 0XA10B:
			strPID =  "SmartSolar MPPT VE.Can 250|100";
			break;
		case 0XA10C:
			strPID =  "SmartSolar MPPT VE.Can 150|70 rev2";
			break;
		case 0XA10D:
			strPID =  "SmartSolar MPPT VE.Can 150|85 rev2";
			break;
		case 0XA10E:
			strPID =  "SmartSolar MPPT VE.Can 150|100 rev2";
			break;
		case 0XA10F:
			strPID =  "BlueSolar MPPT VE.Can 150|100";
			break;
		case 0XA110:
			strPID =  "SmartSolar MPPT RS 450|100";
			break; 
		case 0XA112:
			strPID =  "BlueSolar MPPT VE.Can 250|70";
			break;
		case 0XA113:
			strPID =  "BlueSolar MPPT VE.Can 250|100";
			break;
		case 0XA114:
			strPID =  "SmartSolar MPPT VE.Can 250|70 rev2";
			break;
		case 0XA115:
			strPID =  "SmartSolar MPPT VE.Can 250|100 rev2";
			break;
		case 0XA116:
			strPID =  "SmartSolar MPPT VE.Can 250|85 rev2";
			break;
		default:
			strPID = pid;
	}
	return strPID;
}

/*
 * getCsAsString
 * This function returns the state of operations (CS) as readable text.
 */
String VeDirectFrameHandler::getCsAsString(uint8_t cs)
{
	String strCS ="";

	switch(cs) {
		case 0:
			strCS =  "OFF";
			break;
		case 2:
			strCS =  "Fault";
			break;
		case 3:
			strCS =  "Bulk";
			break;
		case 4:
			strCS =  "Absorbtion";
			break;
		case 5:
			strCS =  "Float";
			break;
		case 7:
			strCS =  "Equalize (manual)";
			break;
		case 245:
			strCS =  "Starting-up";
			break;
		case 247:
			strCS =  "Auto equalize / Recondition";
			break;
		case 252:
			strCS =  "External Control";
			break;
		default:
			strCS = cs;
	}
	return strCS;
}

/*
 * getErrAsString
 * This function returns error state (ERR) as readable text.
 */
String VeDirectFrameHandler::getErrAsString(uint8_t err)
{
	String strERR ="";

	switch(err) {
		case 0:
			strERR =  "No error";
			break;
		case 2:
			strERR =  "Battery voltage too high";
			break;
		case 17:
			strERR =  "Charger temperature too high";
			break;
		case 18:
			strERR =  "Charger over current";
			break;
		case 19:
			strERR =  "Charger current reversed";
			break;
		case 20:
			strERR =  "Bulk time limit exceeded";
			break;
		case 21:
			strERR =  "Current sensor issue(sensor bias/sensor broken)";
			break;
		case 26:
			strERR =  "Terminals overheated";
			break;
		case 28:
			strERR =  "Converter issue (dual converter models only)";
			break;
		case 33:
			strERR =  "Input voltage too high (solar panel)";
			break;
		case 34:
			strERR =  "Input current too high (solar panel)";
			break;
		case 38:
			strERR =  "Input shutdown (due to excessive battery voltage)";
			break;
		case 39:
			strERR =  "Input shutdown (due to current flow during off mode)";
			break;
		case 40:
			strERR =  "Input";
			break;
		case 65:
			strERR =  "Lost communication with one of devices";
			break;
		case 67:
			strERR =  "Synchronisedcharging device configuration issue";
			break;
		case 68:
			strERR =  "BMS connection lost";
			break;
		case 116:
			strERR =  "Factory calibration data lost";
			break;
		case 117:
			strERR =  "Invalid/incompatible firmware";
			break;
		case 118:
			strERR =  "User settings invalid";
			break;
		default:
			strERR = err;
	}
	return strERR;
}

/*
 * getOrAsString
 * This function returns the off reason (OR) as readable text.
 */
String VeDirectFrameHandler::getOrAsString(uint32_t offReason)
{
	String strOR ="";

	switch(offReason) {
		case 0x00000000:
			strOR =  "Not off";
			break;
		case 0x00000001:
			strOR =  "No input power";
			break;
		case 0x00000002:
			strOR =  "Switched off (power switch)";
			break;
		case 0x00000004:
			strOR =  "Switched off (device moderegister)";
			break;
		case 0x00000008:
			strOR =  "Remote input";
			break;
		case 0x00000010:
			strOR =  "Protection active";
			break;
		case 0x00000020:
			strOR =  "Paygo";
			break;
		case 0x00000040:
			strOR =  "BMS";
			break;
		case 0x00000080:
			strOR =  "Engine shutdown detection";
			break;
		case 0x00000100:
			strOR =  "Analysing input voltage";
			break;
		default:
			strOR = offReason;
	}
	return strOR;
}

/*
 * getMpptAsString
 * This function returns the state of MPPT (MPPT) as readable text.
 */
String VeDirectFrameHandler::getMpptAsString(uint8_t mppt)
{
	String strMPPT ="";

	switch(mppt) {
		case 0:
			strMPPT =  "OFF";
			break;
		case 1:
			strMPPT =  "Voltage or current limited";
			break;
		case 2:
			strMPPT =  "MPP Tracker active";
			break;
		default:
			strMPPT = mppt;
	}
	return strMPPT;
}