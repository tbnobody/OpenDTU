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

template<typename T>
VeDirectFrameHandler<T>::VeDirectFrameHandler() :
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

template<typename T>
void VeDirectFrameHandler<T>::init(char const* who, int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging, uint16_t hwSerialPort)
{
	_vedirectSerial = std::make_unique<HardwareSerial>(hwSerialPort);
	_vedirectSerial->begin(19200, SERIAL_8N1, rx, tx);
	_vedirectSerial->flush();
	_msgOut = msgOut;
	_verboseLogging = verboseLogging;
	_debugIn = 0;
	snprintf(_logId, sizeof(_logId), "[VE.Direct %s %d/%d]", who, rx, tx);
	if (_verboseLogging) { _msgOut->printf("%s init complete\r\n", _logId); }
}

template<typename T>
void VeDirectFrameHandler<T>::dumpDebugBuffer() {
	_msgOut->printf("%s serial input (%d Bytes):", _logId, _debugIn);
	for (int i = 0; i < _debugIn; ++i) {
		if (i % 16 == 0) {
			_msgOut->printf("\r\n%s", _logId);
		}
		_msgOut->printf(" %02x", _debugBuffer[i]);
	}
	_msgOut->println("");
	_debugIn = 0;
}

template<typename T>
void VeDirectFrameHandler<T>::reset()
{
	_checksum = 0;
	_state = IDLE;
	_textData.clear();
}

template<typename T>
void VeDirectFrameHandler<T>::loop()
{
	while ( _vedirectSerial->available()) {
		rxData(_vedirectSerial->read());
		_lastByteMillis = millis();
	}

	// there will never be a large gap between two bytes of the same frame.
	// if such a large gap is observed, reset the state machine so it tries
	// to decode a new frame once more data arrives.
	if (IDLE != _state && _lastByteMillis + 500 < millis()) {
		_msgOut->printf("%s Resetting state machine (was %d) after timeout\r\n", _logId, _state);
		if (_verboseLogging) { dumpDebugBuffer(); }
		reset();
	}
}

/*
 *  rxData
 *  This function is called by loop() which passes a byte of serial data
 *  Based on Victron's example code. But using String and Map instead of pointer and arrays
 */
template<typename T>
void VeDirectFrameHandler<T>::rxData(uint8_t inbyte)
{
	if (_verboseLogging) {
		_debugBuffer[_debugIn] = inbyte;
		_debugIn = (_debugIn + 1) % _debugBuffer.size();
		if (0 == _debugIn) {
			_msgOut->printf("%s ERROR: debug buffer overrun!\r\n", _logId);
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
				_textData.push_back({_name, _value});
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
		if (_verboseLogging) { dumpDebugBuffer(); }
		if (_checksum == 0) {
			for (auto const& event : _textData) {
				processTextData(event.first, event.second);
			}
			_lastUpdate = millis();
			frameValidEvent();
		}
		else {
			_msgOut->printf("%s checksum 0x%02x != 0x00, invalid frame\r\n", _logId, _checksum);
		}
		reset();
		break;
	}
	case RECORD_HEX:
		_state = hexRxEvent(inbyte);
		break;
	}
}

/*
 * This function is called every time a new name/value is successfully parsed.  It writes the values to the temporary buffer.
 */
template<typename T>
void VeDirectFrameHandler<T>::processTextData(std::string const& name, std::string const& value) {
	if (_verboseLogging) {
		_msgOut->printf("%s Text Data '%s' = '%s'\r\n",
				_logId, name.c_str(), value.c_str());
	}

	if (processTextDataDerived(name, value)) { return; }

	if (name == "PID") {
		_tmpFrame.PID = strtol(value.c_str(), nullptr, 0);
		return;
	}

	if (name == "SER") {
		strcpy(_tmpFrame.SER, value.c_str());
		return;
	}

	if (name == "FW") {
		strcpy(_tmpFrame.FW, value.c_str());
		return;
	}

	if (name == "V") {
		_tmpFrame.V = round(atof(value.c_str()) / 10.0) / 100.0;
		return;
	}

	if (name == "I") {
		_tmpFrame.I = round(atof(value.c_str()) / 10.0) / 100.0;
		return;
	}

	_msgOut->printf("%s Unknown text data '%s' (value '%s')\r\n",
			_logId, name.c_str(), value.c_str());
}



/*
 *  hexRxEvent
 *  This function records hex answers or async messages
 */
template<typename T>
int VeDirectFrameHandler<T>::hexRxEvent(uint8_t inbyte)
{
	int ret=RECORD_HEX; // default - continue recording until end of frame

	switch (inbyte) {
	case '\n':
		// restore previous state
		ret=_prevState;
		break;

	default:
		_hexSize++;
		if (_hexSize>=VE_MAX_HEX_LEN) { // oops -buffer overflow - something went wrong, we abort
			_msgOut->printf("%s hexRx buffer overflow - aborting read\r\n", _logId);
			_hexSize=0;
			ret=IDLE;
		}
	}

	return ret;
}

template<typename T>
bool VeDirectFrameHandler<T>::isDataValid() const
{
	return strlen(_tmpFrame.SER) > 0 && _lastUpdate > 0 && (millis() - _lastUpdate) < (10 * 1000);
}

template<typename T>
uint32_t VeDirectFrameHandler<T>::getLastUpdate() const
{
	return _lastUpdate;
}
