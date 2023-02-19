// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "AlarmLogParser.h"
#include "../Hoymiles.h"
#include <cstring>

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

    switch (entry->MessageId) {
    case 1:
        entry->Message = F("Inverter start");
        break;
    case 2:
        entry->Message = F("DTU command failed");
        break;
    case 121:
        entry->Message = F("Over temperature protection");
        break;
    case 124:
        entry->Message = F("Shut down by remote control");
        break;
    case 125:
        entry->Message = F("Grid configuration parameter error");
        break;
    case 126:
        entry->Message = F("Software error code 126");
        break;
    case 127:
        entry->Message = F("Firmware error");
        break;
    case 128:
        entry->Message = F("Software error code 128");
        break;
    case 129:
        entry->Message = F("Abnormal bias");
        break;
    case 130:
        entry->Message = F("Offline");
        break;
    case 141:
        entry->Message = F("Grid: Grid overvoltage");
        break;
    case 142:
        entry->Message = F("Grid: 10 min value grid overvoltage");
        break;
    case 143:
        entry->Message = F("Grid: Grid undervoltage");
        break;
    case 144:
        entry->Message = F("Grid: Grid overfrequency");
        break;
    case 145:
        entry->Message = F("Grid: Grid underfrequency");
        break;
    case 146:
        entry->Message = F("Grid: Rapid grid frequency change rate");
        break;
    case 147:
        entry->Message = F("Grid: Power grid outage");
        break;
    case 148:
        entry->Message = F("Grid: Grid disconnection");
        break;
    case 149:
        entry->Message = F("Grid: Island detected");
        break;
    case 205:
        entry->Message = F("MPPT-A: Input overvoltage");
        break;
    case 206:
        entry->Message = F("MPPT-B: Input overvoltage");
        break;
    case 207:
        entry->Message = F("MPPT-A: Input undervoltage");
        break;
    case 208:
        entry->Message = F("MPPT-B: Input undervoltage");
        break;
    case 209:
        entry->Message = F("PV-1: No input");
        break;
    case 210:
        entry->Message = F("PV-2: No input");
        break;
    case 211:
        entry->Message = F("PV-3: No input");
        break;
    case 212:
        entry->Message = F("PV-4: No input");
        break;
    case 213:
        entry->Message = F("MPPT-A: PV-1 & PV-2 abnormal wiring");
        break;
    case 214:
        entry->Message = F("MPPT-B: PV-3 & PV-4 abnormal wiring");
        break;
    case 215:
        entry->Message = F("PV-1: Input overvoltage");
        break;
    case 216:
        entry->Message = F("PV-1: Input undervoltage");
        break;
    case 217:
        entry->Message = F("PV-2: Input overvoltage");
        break;
    case 218:
        entry->Message = F("PV-2: Input undervoltage");
        break;
    case 219:
        entry->Message = F("PV-3: Input overvoltage");
        break;
    case 220:
        entry->Message = F("PV-3: Input undervoltage");
        break;
    case 221:
        entry->Message = F("PV-4: Input overvoltage");
        break;
    case 222:
        entry->Message = F("PV-4: Input undervoltage");
        break;
    case 301:
        entry->Message = F("Hardware error code 301");
        break;
    case 302:
        entry->Message = F("Hardware error code 302");
        break;
    case 303:
        entry->Message = F("Hardware error code 303");
        break;
    case 304:
        entry->Message = F("Hardware error code 304");
        break;
    case 305:
        entry->Message = F("Hardware error code 305");
        break;
    case 306:
        entry->Message = F("Hardware error code 306");
        break;
    case 307:
        entry->Message = F("Hardware error code 307");
        break;
    case 308:
        entry->Message = F("Hardware error code 308");
        break;
    case 309:
        entry->Message = F("Hardware error code 309");
        break;
    case 310:
        entry->Message = F("Hardware error code 310");
        break;
    case 311:
        entry->Message = F("Hardware error code 311");
        break;
    case 312:
        entry->Message = F("Hardware error code 312");
        break;
    case 313:
        entry->Message = F("Hardware error code 313");
        break;
    case 314:
        entry->Message = F("Hardware error code 314");
        break;
    case 5041:
        entry->Message = F("Error code-04 Port 1");
        break;
    case 5042:
        entry->Message = F("Error code-04 Port 2");
        break;
    case 5043:
        entry->Message = F("Error code-04 Port 3");
        break;
    case 5044:
        entry->Message = F("Error code-04 Port 4");
        break;
    case 5051:
        entry->Message = F("PV Input 1 Overvoltage/Undervoltage");
        break;
    case 5052:
        entry->Message = F("PV Input 2 Overvoltage/Undervoltage");
        break;
    case 5053:
        entry->Message = F("PV Input 3 Overvoltage/Undervoltage");
        break;
    case 5054:
        entry->Message = F("PV Input 4 Overvoltage/Undervoltage");
        break;
    case 5060:
        entry->Message = F("Abnormal bias");
        break;
    case 5070:
        entry->Message = F("Over temperature protection");
        break;
    case 5080:
        entry->Message = F("Grid Overvoltage/Undervoltage");
        break;
    case 5090:
        entry->Message = F("Grid Overfrequency/Underfrequency");
        break;
    case 5100:
        entry->Message = F("Island detected");
        break;
    case 5120:
        entry->Message = F("EEPROM reading and writing error");
        break;
    case 5150:
        entry->Message = F("10 min value grid overvoltage");
        break;
    case 5200:
        entry->Message = F("Firmware error");
        break;
    case 8310:
        entry->Message = F("Shut down");
        break;
    case 9000:
        entry->Message = F("Microinverter is suspected of being stolen");
        break;
    default:
        entry->Message = F("Unknown");
        break;
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