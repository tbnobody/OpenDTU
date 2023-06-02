// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
 */
#include "AlarmLogParser.h"
#include "../Hoymiles.h"
#include <cstring>

const std::array<const AlarmMessage_t, ALARM_MSG_COUNT> AlarmLogParser::_alarmMessages = {{
    { AlarmMessageType_t::ALL, 1, "Inverter start" },
    { AlarmMessageType_t::ALL, 2, "DTU command failed" },
    { AlarmMessageType_t::ALL, 73, "Temperature >80°C" }, // https://github.com/tbnobody/OpenDTU/discussions/590#discussioncomment-6049750
    { AlarmMessageType_t::ALL, 121, "Over temperature protection" },
    { AlarmMessageType_t::ALL, 124, "Shut down by remote control" },
    { AlarmMessageType_t::ALL, 125, "Grid configuration parameter error" },
    { AlarmMessageType_t::ALL, 126, "Software error code 126" },
    { AlarmMessageType_t::ALL, 127, "Firmware error" },
    { AlarmMessageType_t::ALL, 128, "Software error code 128" },
    { AlarmMessageType_t::ALL, 129, "Abnormal bias" },
    { AlarmMessageType_t::ALL, 130, "Offline" },
    { AlarmMessageType_t::ALL, 141, "Grid: Grid overvoltage" },
    { AlarmMessageType_t::ALL, 142, "Grid: 10 min value grid overvoltage" },
    { AlarmMessageType_t::ALL, 143, "Grid: Grid undervoltage" },
    { AlarmMessageType_t::ALL, 144, "Grid: Grid overfrequency" },
    { AlarmMessageType_t::ALL, 145, "Grid: Grid underfrequency" },
    { AlarmMessageType_t::ALL, 146, "Grid: Rapid grid frequency change rate" },
    { AlarmMessageType_t::ALL, 147, "Grid: Power grid outage" },
    { AlarmMessageType_t::ALL, 148, "Grid: Grid disconnection" },
    { AlarmMessageType_t::ALL, 149, "Grid: Island detected" },
    { AlarmMessageType_t::HMT, 171, "Grid: Abnormal phase difference between phase to phase" },
    { AlarmMessageType_t::ALL, 205, "MPPT-A: Input overvoltage" },
    { AlarmMessageType_t::ALL, 206, "MPPT-B: Input overvoltage" },
    { AlarmMessageType_t::ALL, 207, "MPPT-A: Input undervoltage" },
    { AlarmMessageType_t::ALL, 208, "MPPT-B: Input undervoltage" },
    { AlarmMessageType_t::ALL, 209, "PV-1: No input" },
    { AlarmMessageType_t::ALL, 210, "PV-2: No input" },
    { AlarmMessageType_t::ALL, 211, "PV-3: No input" },
    { AlarmMessageType_t::ALL, 212, "PV-4: No input" },
    { AlarmMessageType_t::ALL, 213, "MPPT-A: PV-1 & PV-2 abnormal wiring" },
    { AlarmMessageType_t::ALL, 214, "MPPT-B: PV-3 & PV-4 abnormal wiring" },
    { AlarmMessageType_t::ALL, 215, "PV-1: Input overvoltage" },
    { AlarmMessageType_t::HMT, 215, "MPPT-C: Input overvoltage" },
    { AlarmMessageType_t::ALL, 216, "PV-1: Input undervoltage" },
    { AlarmMessageType_t::HMT, 216, "MPPT-C: Input undervoltage" },
    { AlarmMessageType_t::ALL, 217, "PV-2: Input overvoltage" },
    { AlarmMessageType_t::HMT, 217, "PV-5: No input" },
    { AlarmMessageType_t::ALL, 218, "PV-2: Input undervoltage" },
    { AlarmMessageType_t::HMT, 218, "PV-6: No input" },
    { AlarmMessageType_t::ALL, 219, "PV-3: Input overvoltage" },
    { AlarmMessageType_t::HMT, 219, "MPPT-C: PV-5 & PV-6 abnormal wiring" },
    { AlarmMessageType_t::ALL, 220, "PV-3: Input undervoltage" },
    { AlarmMessageType_t::ALL, 221, "PV-4: Input overvoltage" },
    { AlarmMessageType_t::HMT, 221, "Abnormal wiring of grid neutral line" },
    { AlarmMessageType_t::ALL, 222, "PV-4: Input undervoltage" },
    { AlarmMessageType_t::ALL, 301, "Hardware error code 301" },
    { AlarmMessageType_t::ALL, 302, "Hardware error code 302" },
    { AlarmMessageType_t::ALL, 303, "Hardware error code 303" },
    { AlarmMessageType_t::ALL, 304, "Hardware error code 304" },
    { AlarmMessageType_t::ALL, 305, "Hardware error code 305" },
    { AlarmMessageType_t::ALL, 306, "Hardware error code 306" },
    { AlarmMessageType_t::ALL, 307, "Hardware error code 307" },
    { AlarmMessageType_t::ALL, 308, "Hardware error code 308" },
    { AlarmMessageType_t::ALL, 309, "Hardware error code 309" },
    { AlarmMessageType_t::ALL, 310, "Hardware error code 310" },
    { AlarmMessageType_t::ALL, 311, "Hardware error code 311" },
    { AlarmMessageType_t::ALL, 312, "Hardware error code 312" },
    { AlarmMessageType_t::ALL, 313, "Hardware error code 313" },
    { AlarmMessageType_t::ALL, 314, "Hardware error code 314" },
    { AlarmMessageType_t::ALL, 5041, "Error code-04 Port 1" },
    { AlarmMessageType_t::ALL, 5042, "Error code-04 Port 2" },
    { AlarmMessageType_t::ALL, 5043, "Error code-04 Port 3" },
    { AlarmMessageType_t::ALL, 5044, "Error code-04 Port 4" },
    { AlarmMessageType_t::ALL, 5051, "PV Input 1 Overvoltage/Undervoltage" },
    { AlarmMessageType_t::ALL, 5052, "PV Input 2 Overvoltage/Undervoltage" },
    { AlarmMessageType_t::ALL, 5053, "PV Input 3 Overvoltage/Undervoltage" },
    { AlarmMessageType_t::ALL, 5054, "PV Input 4 Overvoltage/Undervoltage" },
    { AlarmMessageType_t::ALL, 5060, "Abnormal bias" },
    { AlarmMessageType_t::ALL, 5070, "Over temperature protection" },
    { AlarmMessageType_t::ALL, 5080, "Grid Overvoltage/Undervoltage" },
    { AlarmMessageType_t::ALL, 5090, "Grid Overfrequency/Underfrequency" },
    { AlarmMessageType_t::ALL, 5100, "Island detected" },
    { AlarmMessageType_t::ALL, 5120, "EEPROM reading and writing error" },
    { AlarmMessageType_t::ALL, 5150, "10 min value grid overvoltage" },
    { AlarmMessageType_t::ALL, 5200, "Firmware error" },
    { AlarmMessageType_t::ALL, 8310, "Shut down" },
    { AlarmMessageType_t::ALL, 9000, "Microinverter is suspected of being stolen" },
}};

void AlarmLogParser::clearBuffer()
{
    memset(_payloadAlarmLog, 0, ALARM_LOG_PAYLOAD_SIZE);
    _alarmLogLength = 0;
}

void AlarmLogParser::appendFragment(uint8_t offset, uint8_t* payload, uint8_t len)
{
    if (offset + len > ALARM_LOG_PAYLOAD_SIZE) {
        Hoymiles.getMessageOutput()->printf("FATAL: (%s, %d) stats packet too large for buffer (%d > %d)\r\n", __FILE__, __LINE__, offset + len, ALARM_LOG_PAYLOAD_SIZE);
        return;
    }
    memcpy(&_payloadAlarmLog[offset], payload, len);
    _alarmLogLength += len;
}

uint8_t AlarmLogParser::getEntryCount()
{
    return (_alarmLogLength - 2) / ALARM_LOG_ENTRY_SIZE;
}

void AlarmLogParser::setLastAlarmRequestSuccess(LastCommandSuccess status)
{
    _lastAlarmRequestSuccess = status;
}

LastCommandSuccess AlarmLogParser::getLastAlarmRequestSuccess()
{
    return _lastAlarmRequestSuccess;
}

void AlarmLogParser::setMessageType(AlarmMessageType_t type)
{
    _messageType = type;
}

void AlarmLogParser::getLogEntry(uint8_t entryId, AlarmLogEntry_t* entry)
{
    uint8_t entryStartOffset = 2 + entryId * ALARM_LOG_ENTRY_SIZE;

    int timezoneOffset = getTimezoneOffset();

    uint32_t wcode = (uint16_t)_payloadAlarmLog[entryStartOffset] << 8 | _payloadAlarmLog[entryStartOffset + 1];
    uint32_t startTimeOffset = 0;
    if (((wcode >> 13) & 0x01) == 1) {
        startTimeOffset = 12 * 60 * 60;
    }

    uint32_t endTimeOffset = 0;
    if (((wcode >> 12) & 0x01) == 1) {
        endTimeOffset = 12 * 60 * 60;
    }

    entry->MessageId = _payloadAlarmLog[entryStartOffset + 1];
    entry->StartTime = (((uint16_t)_payloadAlarmLog[entryStartOffset + 4] << 8) | ((uint16_t)_payloadAlarmLog[entryStartOffset + 5])) + startTimeOffset + timezoneOffset;
    entry->EndTime = ((uint16_t)_payloadAlarmLog[entryStartOffset + 6] << 8) | ((uint16_t)_payloadAlarmLog[entryStartOffset + 7]);

    if (entry->EndTime > 0) {
        entry->EndTime += (endTimeOffset + timezoneOffset);
    }

    entry->Message = "Unknown";
    for (auto& msg : _alarmMessages) {
        if (msg.MessageId == entry->MessageId) {
            if (msg.InverterType == _messageType) {
                entry->Message = msg.Message;
                break;
            } else if (msg.InverterType == AlarmMessageType_t::ALL) {
                entry->Message = msg.Message;
            }
        }
    }
}

int AlarmLogParser::getTimezoneOffset()
{
    // see: https://stackoverflow.com/questions/13804095/get-the-time-zone-gmt-offset-in-c/44063597#44063597

    time_t gmt, rawtime = time(NULL);
    struct tm* ptm;

    struct tm gbuf;
    ptm = gmtime_r(&rawtime, &gbuf);

    // Request that mktime() looksup dst in timezone database
    ptm->tm_isdst = -1;
    gmt = mktime(ptm);

    return static_cast<int>(difftime(rawtime, gmt));
}