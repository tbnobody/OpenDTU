// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2026 Thomas Basler and others
 */

/*
This command is used to fetch the eventlog from the inverter.

Derives from MultiDataCommand

Command structure:
* DT: this specific command uses 0x11
* Sel: sub-selector byte. Unused for 0x11, send 0x00.
* Time: unix time. If it differs from the inverter clock by 5 s or more, the inverter
  re-syncs its clock and shifts the stored event times by the difference.
* Gap: averaging interval in minutes for the interval record. 0 keeps the current interval
  (default 15 min).
* AlarmId: the SeqIdx of the last event already received. The inverter returns only entries
  whose SeqIdx is greater; 0 returns the whole log. On the first request after inverter boot
  it continues its event numbering from AlarmId. --> Not implemented here (always sent as 0)

00   01 02 03 04   05 06 07 08   09   10   11   12 13 14 15   16 17   18 19   20 21 22 23   24 25   26   27 28 29 30 31
-----------------------------------------------------------------------------------------------------------------------
                                      |<------------------- CRC16 --------------------->|
15   71 60 35 46   80 12 23 04   80   11   00   65 72 06 B8   00 00   00 00   00 00 00 00   00 00   00   -- -- -- -- --
^^   ^^^^^^^^^^^   ^^^^^^^^^^^   ^^   ^^   ^^   ^^^^^^^^^^^   ^^^^^   ^^^^^   ^^^^^^^^^^^   ^^^^^   ^^
ID   Target Addr   Source Addr   Idx  DT   Sel  Time          Gap     AlarmId Password      CRC16   CRC8
*/
#include "AlarmDataCommand.h"
#include "inverters/InverterAbstract.h"

AlarmDataCommand::AlarmDataCommand(InverterAbstract* inv, const uint64_t router_address, const time_t time)
    : MultiDataCommand(inv, router_address)
{
    setTime(time);
    setDataType(0x11);
    setTimeout(750);
}

String AlarmDataCommand::getCommandName() const
{
    return "AlarmData";
}

bool AlarmDataCommand::handleResponse(const fragment_t fragment[], const uint8_t max_fragment_id)
{
    // Check CRC of whole payload
    if (!MultiDataCommand::handleResponse(fragment, max_fragment_id)) {
        return false;
    }

    // Move all fragments into target buffer
    uint8_t offs = 0;
    _inv->EventLog()->beginAppendFragment();
    _inv->EventLog()->clearBuffer();
    for (uint8_t i = 0; i < max_fragment_id; i++) {
        _inv->EventLog()->appendFragment(offs, fragment[i].fragment, fragment[i].len);
        offs += (fragment[i].len);
    }
    _inv->EventLog()->endAppendFragment();
    _inv->EventLog()->setLastAlarmRequestSuccess(CMD_OK);
    _inv->EventLog()->setLastUpdate(millis());
    return true;
}

void AlarmDataCommand::gotTimeout()
{
    _inv->EventLog()->setLastAlarmRequestSuccess(CMD_NOK);
}
