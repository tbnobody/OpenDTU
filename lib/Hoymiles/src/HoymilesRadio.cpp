// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "HoymilesRadio.h"
#include "Hoymiles.h"
#include "crc.h"

serial_u HoymilesRadio::DtuSerial()
{
    return _dtuSerial;
}

void HoymilesRadio::setDtuSerial(uint64_t serial)
{
    _dtuSerial.u64 = serial;
}

serial_u HoymilesRadio::convertSerialToRadioId(serial_u serial)
{
    serial_u radioId;
    radioId.u64 = 0;
    radioId.b[4] = serial.b[0];
    radioId.b[3] = serial.b[1];
    radioId.b[2] = serial.b[2];
    radioId.b[1] = serial.b[3];
    radioId.b[0] = 0x01;
    return radioId;
}

bool HoymilesRadio::checkFragmentCrc(fragment_t* fragment)
{
    uint8_t crc = crc8(fragment->fragment, fragment->len - 1);
    return (crc == fragment->fragment[fragment->len - 1]);
}

void HoymilesRadio::sendRetransmitPacket(uint8_t fragment_id)
{
    CommandAbstract* cmd = _commandQueue.front().get();

    CommandAbstract* requestCmd = cmd->getRequestFrameCommand(fragment_id);

    if (requestCmd != nullptr) {
        sendEsbPacket(requestCmd);
    }
}

void HoymilesRadio::sendLastPacketAgain()
{
    CommandAbstract* cmd = _commandQueue.front().get();
    sendEsbPacket(cmd);
}

void HoymilesRadio::dumpBuf(const uint8_t buf[], uint8_t len, bool appendNewline)
{
    for (uint8_t i = 0; i < len; i++) {
        Hoymiles.getMessageOutput()->printf("%02X ", buf[i]);
    }
    if (appendNewline) {
        Hoymiles.getMessageOutput()->println("");
    }
}

bool HoymilesRadio::isInitialized()
{
    return _isInitialized;
}

bool HoymilesRadio::isIdle()
{
    return !_busyFlag;
}