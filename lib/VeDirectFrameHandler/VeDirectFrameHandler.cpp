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



class Silent : public Print {
	public:
		size_t write(uint8_t c) final { return 0; }
};

static Silent MessageOutputDummy;

VeDirectFrameHandler::VeDirectFrameHandler() :
	_msgOut(&MessageOutputDummy),
	_lastUpdate(0),
	_state(IDLE),
	_checksum(0),
	_textPointer(0),
	_hexSize(0),
	_name(""),
	_value(""),
	_debugIn(0),
	_lastByteMillis(0)
{
}

void VeDirectFrameHandler::init(int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging, uint16_t hwSerialPort)
{
	_vedirectSerial = std::make_unique<HardwareSerial>(hwSerialPort);
	_vedirectSerial->begin(19200, SERIAL_8N1, rx, tx);
	_vedirectSerial->flush();
	_msgOut = msgOut;
	_verboseLogging = verboseLogging;
	_debugIn = 0;
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
	while ( _vedirectSerial->available()) {
		rxData(_vedirectSerial->read());
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
	}
}

/*
 *  rxData
 *  This function is called by loop() which passes a byte of serial data
 *  Based on Victron's example code. But using String and Map instead of pointer and arrays
 */
void VeDirectFrameHandler::rxData(uint8_t inbyte)
{
	if (_verboseLogging) {
		_debugBuffer[_debugIn] = inbyte;
		_debugIn = (_debugIn + 1) % _debugBuffer.size();
		if (0 == _debugIn) {
			_msgOut->println("[VE.Direct] ERROR: debug buffer overrun!");
		}
	}

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
		if (valid) { frameValidEvent(); }
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
bool VeDirectFrameHandler::textRxEvent(std::string const& who, char* name, char* value, veStruct& frame) {
	if (_verboseLogging) {
		_msgOut->printf("[Victron %s] Text Event %s: Value: %s\r\n",
				who.c_str(), name, value );
	}

	if (strcmp(name, "PID") == 0) {
		frame.PID = strtol(value, nullptr, 0);
		return true;
	}

	if (strcmp(name, "SER") == 0) {
		strcpy(frame.SER, value);
		return true;
	}

	if (strcmp(name, "FW") == 0) {
		strcpy(frame.FW, value);
		return true;
	}

	if (strcmp(name, "V") == 0) {
		frame.V = round(atof(value) / 10.0) / 100.0;
		return true;
	}

	if (strcmp(name, "I") == 0) {
		frame.I = round(atof(value) / 10.0) / 100.0;
		return true;
	}

	return false;
}



/*
 *  hexRxEvent
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

bool VeDirectFrameHandler::isDataValid(veStruct const& frame) const {
	if (_lastUpdate == 0) {
		return false;
	}
	if (strlen(frame.SER) == 0) {
		return false;
	}
	return true;
}

uint32_t VeDirectFrameHandler::getLastUpdate() const
{
	return _lastUpdate;
}

template<typename T>
String const& VeDirectFrameHandler::getAsString(std::map<T, String> const& values, T val)
{
	auto pos = values.find(val);
	if (pos == values.end()) {
		static String dummy;
		dummy = val;
		return dummy;
	}
	return pos->second;
}

template String const& VeDirectFrameHandler::getAsString(std::map<uint8_t, String> const& values, uint8_t val);
template String const& VeDirectFrameHandler::getAsString(std::map<uint16_t, String> const& values, uint16_t val);
template String const& VeDirectFrameHandler::getAsString(std::map<uint32_t, String> const& values, uint32_t val);

/*
 * getPidAsString
 * This function returns the product id (PID) as readable text.
 */
String VeDirectFrameHandler::veStruct::getPidAsString() const
{
	static const std::map<uint16_t, String> values = {
		{ 0x0300, F("BlueSolar MPPT 70|15") },
		{ 0xA040, F("BlueSolar MPPT 75|50") },
		{ 0xA041, F("BlueSolar MPPT 150|35") },
		{ 0xA042, F("BlueSolar MPPT 75|15") },
		{ 0xA043, F("BlueSolar MPPT 100|15") },
		{ 0xA044, F("BlueSolar MPPT 100|30") },
		{ 0xA045, F("BlueSolar MPPT 100|50") },
		{ 0xA046, F("BlueSolar MPPT 100|70") },
		{ 0xA047, F("BlueSolar MPPT 150|100") },
		{ 0xA049, F("BlueSolar MPPT 100|50 rev2") },
		{ 0xA04A, F("BlueSolar MPPT 100|30 rev2") },
		{ 0xA04B, F("BlueSolar MPPT 150|35 rev2") },
		{ 0xA04C, F("BlueSolar MPPT 75|10") },
		{ 0xA04D, F("BlueSolar MPPT 150|45") },
		{ 0xA04E, F("BlueSolar MPPT 150|60") },
		{ 0xA04F, F("BlueSolar MPPT 150|85") },
		{ 0xA050, F("SmartSolar MPPT 250|100") },
		{ 0xA051, F("SmartSolar MPPT 150|100") },
		{ 0xA052, F("SmartSolar MPPT 150|85") },
		{ 0xA053, F("SmartSolar MPPT 75|15") },
		{ 0xA054, F("SmartSolar MPPT 75|10") },
		{ 0xA055, F("SmartSolar MPPT 100|15") },
		{ 0xA056, F("SmartSolar MPPT 100|30") },
		{ 0xA057, F("SmartSolar MPPT 100|50") },
		{ 0xA058, F("SmartSolar MPPT 150|35") },
		{ 0xA059, F("SmartSolar MPPT 150|10 rev2") },
		{ 0xA05A, F("SmartSolar MPPT 150|85 rev2") },
		{ 0xA05B, F("SmartSolar MPPT 250|70") },
		{ 0xA05C, F("SmartSolar MPPT 250|85") },
		{ 0xA05D, F("SmartSolar MPPT 250|60") },
		{ 0xA05E, F("SmartSolar MPPT 250|45") },
		{ 0xA05F, F("SmartSolar MPPT 100|20") },
		{ 0xA060, F("SmartSolar MPPT 100|20 48V") },
		{ 0xA061, F("SmartSolar MPPT 150|45") },
		{ 0xA062, F("SmartSolar MPPT 150|60") },
		{ 0xA063, F("SmartSolar MPPT 150|70") },
		{ 0xA064, F("SmartSolar MPPT 250|85 rev2") },
		{ 0xA065, F("SmartSolar MPPT 250|100 rev2") },
		{ 0xA066, F("BlueSolar MPPT 100|20") },
		{ 0xA067, F("BlueSolar MPPT 100|20 48V") },
		{ 0xA068, F("SmartSolar MPPT 250|60 rev2") },
		{ 0xA069, F("SmartSolar MPPT 250|70 rev2") },
		{ 0xA06A, F("SmartSolar MPPT 150|45 rev2") },
		{ 0xA06B, F("SmartSolar MPPT 150|60 rev2") },
		{ 0xA06C, F("SmartSolar MPPT 150|70 rev2") },
		{ 0xA06D, F("SmartSolar MPPT 150|85 rev3") },
		{ 0xA06E, F("SmartSolar MPPT 150|100 rev3") },
		{ 0xA06F, F("BlueSolar MPPT 150|45 rev2") },
		{ 0xA070, F("BlueSolar MPPT 150|60 rev2") },
		{ 0xA071, F("BlueSolar MPPT 150|70 rev2") },
		{ 0xA102, F("SmartSolar MPPT VE.Can 150|70") },
		{ 0xA103, F("SmartSolar MPPT VE.Can 150|45") },
		{ 0xA104, F("SmartSolar MPPT VE.Can 150|60") },
		{ 0xA105, F("SmartSolar MPPT VE.Can 150|85") },
		{ 0xA106, F("SmartSolar MPPT VE.Can 150|100") },
		{ 0xA107, F("SmartSolar MPPT VE.Can 250|45") },
		{ 0xA108, F("SmartSolar MPPT VE.Can 250|60") },
		{ 0xA109, F("SmartSolar MPPT VE.Can 250|80") },
		{ 0xA10A, F("SmartSolar MPPT VE.Can 250|85") },
		{ 0xA10B, F("SmartSolar MPPT VE.Can 250|100") },
		{ 0xA10C, F("SmartSolar MPPT VE.Can 150|70 rev2") },
		{ 0xA10D, F("SmartSolar MPPT VE.Can 150|85 rev2") },
		{ 0xA10E, F("SmartSolar MPPT VE.Can 150|100 rev2") },
		{ 0xA10F, F("BlueSolar MPPT VE.Can 150|100") },
		{ 0xA110, F("SmartSolar MPPT RS 450|100") },
		{ 0xA112, F("BlueSolar MPPT VE.Can 250|70") },
		{ 0xA113, F("BlueSolar MPPT VE.Can 250|100") },
		{ 0xA114, F("SmartSolar MPPT VE.Can 250|70 rev2") },
		{ 0xA115, F("SmartSolar MPPT VE.Can 250|100 rev2") },
		{ 0xA116, F("SmartSolar MPPT VE.Can 250|85 rev2") },
		{ 0xA381, F("BMV-712 Smart") },
		{ 0xA382, F("BMV-710H Smart") },
		{ 0xA383, F("BMV-712 Smart Rev2") },
		{ 0xA389, F("SmartShunt 500A/50mV") },
		{ 0xA38A, F("SmartShunt 1000A/50mV") },
		{ 0xA38B, F("SmartShunt 2000A/50mV") },
		{ 0xA3F0, F("SmartShunt 2000A/50mV" ) }
	};

	return getAsString(values, PID);
}
