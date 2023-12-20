// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
 */

/*
Derives from CommandAbstract. Has a fixed length of 26 bytes.

Command structure:
* ID: fixed identifier and everytime 0x15
* Idx: the counter of sequencial packages to send. Currently it's only 0x80
  because all request requests only consist of one package.
* DT: repressents the data type and specifies which sub-command to be fetched
* Time: represents the current unix timestamp as hex format. The time on the inverter is synced to the sent time.
  Can be calculated e.g. using the following command
    echo "obase=16; $(date --date='2023-12-07 18:54:00' +%s)" | bc
* Gap: always 0x0
* Password: currently always 0x0
* CRC16: calcuclated over the highlighted amount of bytes

00   01 02 03 04   05 06 07 08   09   10   11   12 13 14 15   16 17   18 19   20 21 22 23   24 25   26   27 28 29 30 31
-----------------------------------------------------------------------------------------------------------------------
                                      |<------------------- CRC16 --------------------->|
15   71 60 35 46   80 12 23 04   80   00   00   65 72 06 B8   00 00   00 00   00 00 00 00   00 00   00   -- -- -- -- --
^^   ^^^^^^^^^^^   ^^^^^^^^^^^   ^^   ^^   ^^   ^^^^^^^^^^^   ^^^^^           ^^^^^^^^^^^   ^^^^^   ^^
ID   Target Addr   Source Addr   Idx  DT   ?    Time          Gap             Password      CRC16   CRC8
*/
#include "MultiDataCommand.h"
#include "crc.h"

MultiDataCommand::MultiDataCommand(const uint64_t target_address, const uint64_t router_address, const uint8_t data_type, const time_t time)
    : CommandAbstract(target_address, router_address)
{
    _payload[0] = 0x15;
    _payload[9] = 0x80;
    setDataType(data_type);
    _payload[11] = 0x00;
    setTime(time);
    _payload[16] = 0x00; // Gap
    _payload[17] = 0x00; // Gap
    _payload[18] = 0x00;
    _payload[19] = 0x00;
    _payload[20] = 0x00; // Password
    _payload[21] = 0x00; // Password
    _payload[22] = 0x00; // Password
    _payload[23] = 0x00; // Password

    udpateCRC();

    _payload_size = 26;
}

void MultiDataCommand::setDataType(const uint8_t data_type)
{
    _payload[10] = data_type;
    udpateCRC();
}
uint8_t MultiDataCommand::getDataType() const
{
    return _payload[10];
}

void MultiDataCommand::setTime(const time_t time)
{
    _payload[12] = (uint8_t)(time >> 24);
    _payload[13] = (uint8_t)(time >> 16);
    _payload[14] = (uint8_t)(time >> 8);
    _payload[15] = (uint8_t)(time);
    udpateCRC();
}

time_t MultiDataCommand::getTime() const
{
    return (time_t)(_payload[12] << 24)
        | (time_t)(_payload[13] << 16)
        | (time_t)(_payload[14] << 8)
        | (time_t)(_payload[15]);
}

CommandAbstract* MultiDataCommand::getRequestFrameCommand(const uint8_t frame_no)
{
    _cmdRequestFrame.setTargetAddress(getTargetAddress());
    _cmdRequestFrame.setFrameNo(frame_no);

    return &_cmdRequestFrame;
}

bool MultiDataCommand::handleResponse(InverterAbstract& inverter, const fragment_t fragment[], const uint8_t max_fragment_id)
{
    // All fragments are available --> Check CRC
    uint16_t crc = 0xffff, crcRcv = 0;

    for (uint8_t i = 0; i < max_fragment_id; i++) {
        // Doublecheck if correct answer package
        if (fragment[i].mainCmd != (_payload[0] | 0x80)) {
            return false;
        }

        if (i == max_fragment_id - 1) {
            // Last packet
            crc = crc16(fragment[i].fragment, fragment[i].len - 2, crc);
            crcRcv = (fragment[i].fragment[fragment[i].len - 2] << 8)
                | (fragment[i].fragment[fragment[i].len - 1]);
        } else {
            crc = crc16(fragment[i].fragment, fragment[i].len, crc);
        }
    }

    return crc == crcRcv;
}

void MultiDataCommand::udpateCRC()
{
    const  uint16_t crc = crc16(&_payload[10], 14); // From data_type till password
    _payload[24] = (uint8_t)(crc >> 8);
    _payload[25] = (uint8_t)(crc);
}

uint8_t MultiDataCommand::getTotalFragmentSize(const fragment_t fragment[], const uint8_t max_fragment_id)
{
    uint8_t fragmentSize = 0;
    for (uint8_t i = 0; i < max_fragment_id; i++) {
        fragmentSize += fragment[i].len;
    }
    return fragmentSize;
}
