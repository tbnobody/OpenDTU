/* VeDirectFrame
HexHandler.cpp
 *
 * Library to read/write from Victron devices using VE.Direct Hex protocol.
 * Add on to Victron framehandler reference implementation.
 *
 * How to use:
 * 1. Use sendHexCommand() to send hex messages. Use the Victron documentation to find the parameter.
 * 2. The from class "VeDirectFrameHandler" derived class X must overwrite the function
 *    void VeDirectFrameHandler::hexDataHandler(VeDirectHexData const &data)
 *    to handle the received hex messages. All hex messages will be forwarded to function hexDataHandler()
 * 3. Analyse the content of data (struct VeDirectHexData) to check if a message fits.
 *
 * 2024.03.08 - 0.4 - adds the ability to send hex commands and to parse hex messages
 *
 */
#include <Arduino.h>
#include "VeDirectFrameHandler.h"

/*
 * calcHexFrameCheckSum()
 * help function to calculate the hex checksum
 */
#define ascii2hex(v) (v-48-(v>='A'?7:0))
#define hex2byte(b) (ascii2hex(*(b)))*16+((ascii2hex(*(b+1))))
static uint8_t calcHexFrameCheckSum(const char* buffer, int size) {
    uint8_t checksum=0x55-ascii2hex(buffer[1]);
    for (int i=2; i<size; i+=2)
        checksum -= hex2byte(buffer+i);
    return (checksum);
}


/*
 * AsciiHexLE2Int()
 * help function to convert AsciiHex Little Endian to uint32_t
 * ascii:   pointer to Ascii Hex Little Endian data
 * anz:     1,2,4 or 8 nibble
 */
static uint32_t AsciiHexLE2Int(const char *ascii, const uint8_t anz) {
    char help[9] = {};

    // sort from little endian format to normal format
    switch (anz) {
        case 1:
            help[0] = ascii[0];
            break;
        case 2:
        case 4:
        case 8:
            for (uint8_t i = 0; i < anz; i += 2) {
                help[i] = ascii[anz-i-2];
                help[i+1] = ascii[anz-i-1];
            }
        default:
            break;
    }
    return (static_cast<uint32_t>(strtoul(help, nullptr, 16)));
}


/*
 * disassembleHexData()
 * analysis the hex message and extract: response, address, flags and value/text
 * buffer:  pointer to message (ascii hex little endian format)
 * data:    disassembled message
 * return:  true = successful disassembled, false = hex sum fault or message
 *          do not align with VE.Direct syntax
 */
template<typename T>
bool VeDirectFrameHandler<T>::disassembleHexData(VeDirectHexData &data) {
    bool state = false;
    char * buffer = _hexBuffer;
    auto len = strlen(buffer);

    // reset hex data first
    data = {};

    if ((len > 3) && (calcHexFrameCheckSum(buffer, len) == 0x00)) {
        data.rsp = static_cast<VeDirectHexResponse>(AsciiHexLE2Int(buffer+1, 1));

        using Response = VeDirectHexResponse;
        switch (data.rsp) {
            case Response::DONE:
            case Response::ERROR:
            case Response::PING:
            case Response::UNKNOWN:
                strncpy(data.text, buffer+2, len-4);
                state = true;
                break;
            case Response::GET:
            case Response::SET:
            case Response::ASYNC:
                data.addr = static_cast<VeDirectHexRegister>(AsciiHexLE2Int(buffer+2, 4));

                // future option: Up to now we do not use historical data
                if ((data.addr >= VeDirectHexRegister::HistoryTotal) && (data.addr <= VeDirectHexRegister::HistoryMPPTD30)) {
                    state = true;
                    break;
                }

                // future option: to analyse the flags here?
                data.flags = AsciiHexLE2Int(buffer+6, 2);

               if (len == 12) {    // 8bit value
                    data.value = AsciiHexLE2Int(buffer+8, 2);
                    state = true;
                }

                if (len == 14) {    // 16bit value
                    data.value = AsciiHexLE2Int(buffer+8, 4);
                    state = true;
                }

                if (len == 18) {    // 32bit value
                    data.value = AsciiHexLE2Int(buffer+8, 8);
                    state = true;
                }
                break;
            default:
                break;  // something went wrong
        }
    }

    if (!state)
        _msgOut->printf("%s failed to disassemble the hex message: %s\r\n", _logId, buffer);

    return (state);
}


/*
 * uint2toHexLEString()
 * help function to convert up to 32 bits into little endian hex String
 * ascii:   pointer to Ascii Hex Little Endian data
 * anz:     1,2,4 or 8 nibble
 */
static String Int2HexLEString(uint32_t value, uint8_t anz) {
    char hexchar[] = "0123456789ABCDEF";
    char help[9] = {};

    switch (anz) {
        case 1:
            help[0] = hexchar[(value & 0x0000000F)];
            break;
        case 2:
        case 4:
        case 8:
            for (uint8_t i = 0; i < anz; i += 2) {
                help[i] = hexchar[(value>>((1+1*i)*4)) & 0x0000000F];
                help[i+1] = hexchar[(value>>((1*i)*4)) & 0x0000000F];
            }
        default:
            ;
    }
    return String(help);
}


/*
 * sendHexCommand()
 * send the hex commend after assembling the command string
 * cmd:     command
 * addr:    register address, default 0
 * value:   value to write into a register, default 0
 * valsize: size of the value, 8, 16 or 32 bit, default 0
 * return:  true = message assembled and send, false = it was not possible to put the message together
 * SAMPLE:  ping command: sendHexCommand(PING),
 *          read total DC input power sendHexCommand(GET, 0xEDEC)
 *          set Charge current limit 10A sendHexCommand(SET, 0x2015, 64, 16)
 *
 * WARNING: some values are stored in non-volatile memory. Continuous writing, for example from a control loop, will
 *          lead to early failure.
 *          On MPPT for example 0xEDE0 - 0xEDFF. Check the Victron doc "BlueSolar-HEX-protocol.pdf"
 */
template<typename T>
bool VeDirectFrameHandler<T>::sendHexCommand(VeDirectHexCommand cmd, VeDirectHexRegister addr, uint32_t value, uint8_t valsize) {
    bool ret = false;
    uint8_t flags = 0x00;  // always 0x00

    String txData = ":" + Int2HexLEString(static_cast<uint32_t>(cmd), 1);    // add the command nibble

    using Command = VeDirectHexCommand;
    switch (cmd) {
    case Command::PING:
    case Command::APP_VERSION:
    case Command::PRODUCT_ID:
        ret = true;
        break;
    case Command::GET:
    case Command::ASYNC:
        txData += Int2HexLEString(static_cast<uint16_t>(addr), 4);
        txData += Int2HexLEString(flags, 2);         // add the flags (2 nibble)
        ret = true;
        break;
    case Command::SET:
        txData += Int2HexLEString(static_cast<uint16_t>(addr), 4);
        txData += Int2HexLEString(flags, 2);         // add the flags (2 nibble)
        if ((valsize == 8) || (valsize == 16) || (valsize == 32)) {
            txData += Int2HexLEString(value, valsize/4);     // add value (2-8 nibble)
            ret = true;
        }
        break;
    default:
        ret = false;
        break;
    }

    if (ret) {
        // add the checksum (2 nibble)
        txData += Int2HexLEString(calcHexFrameCheckSum(txData.c_str(), txData.length()), 2);
        String send = txData + "\n";                // hex command end byte
        _vedirectSerial->write(send.c_str(), send.length());

        if (_verboseLogging) {
            auto blen = _vedirectSerial->availableForWrite();
            _msgOut->printf("%s Sending Hex Command: %s, Free FIFO-Buffer: %u\r\n",
                _logId, txData.c_str(), blen);
        }
    }

    if (!ret)
        _msgOut->printf("%s send hex command fault: %s\r\n", _logId, txData.c_str());

    return (ret);
}
