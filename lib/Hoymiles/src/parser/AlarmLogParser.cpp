#include "AlarmLogParser.h"
#include <cstring>

void AlarmLogParser::clearBuffer()
{
    memset(_payloadAlarmLog, 0, ALARM_LOG_ENTRY_COUNT * ALARM_LOG_ENTRY_SIZE);
    _alarmLogLength = 0;
}

void AlarmLogParser::appendFragment(uint8_t offset, uint8_t* payload, uint8_t len)
{
    if (offset + len > (ALARM_LOG_ENTRY_COUNT * ALARM_LOG_ENTRY_SIZE)) {
        Serial.printf("FATAL: (%s, %d) stats packet too large for buffer\n", __FILE__, __LINE__);
        return;
    }
    memcpy(&_payloadAlarmLog[offset], payload, len);
    _alarmLogLength += len;
}

uint8_t AlarmLogParser::getEntryCount()
{
    return (_alarmLogLength - 2) / ALARM_LOG_ENTRY_SIZE;
}

void AlarmLogParser::getLogEntry(uint8_t entryId, AlarmLogEntry_t* entry)
{
    uint8_t entryStartOffset = 2 + entryId * ALARM_LOG_ENTRY_SIZE;

    int timezoneOffset = getTimezoneOffset();

    entry->MessageId = _payloadAlarmLog[entryStartOffset + 1];
    entry->StartTime = ((uint16_t)_payloadAlarmLog[entryStartOffset + 4] << 8) | ((uint16_t)_payloadAlarmLog[entryStartOffset + 5]) + timezoneOffset;
    entry->EndTime = ((uint16_t)_payloadAlarmLog[entryStartOffset + 6] << 8) | ((uint16_t)_payloadAlarmLog[entryStartOffset + 7]);

    if (entry->EndTime > 0) {
        entry->EndTime += timezoneOffset;
    }

    switch (entry->MessageId) {
    case 1:
        entry->Message = String(F("Inverter start"));
        break;
    case 2:
        entry->Message = String(F("DTU command failed"));
        break;
    case 121:
        entry->Message = String(F("Over temperature protection"));
        break;
    case 125:
        entry->Message = String(F("Grid configuration parameter error"));
        break;
    case 126:
        entry->Message = String(F("Software error code 126"));
        break;
    case 127:
        entry->Message = String(F("Firmware error"));
        break;
    case 128:
        entry->Message = String(F("Software error code 128"));
        break;
    case 129:
        entry->Message = String(F("Software error code 129"));
        break;
    case 130:
        entry->Message = String(F("Offline"));
        break;
    case 141:
        entry->Message = String(F("Grid overvoltage"));
        break;
    case 142:
        entry->Message = String(F("Average grid overvoltage"));
        break;
    case 143:
        entry->Message = String(F("Grid undervoltage"));
        break;
    case 144:
        entry->Message = String(F("Grid overfrequency"));
        break;
    case 145:
        entry->Message = String(F("Grid underfrequency"));
        break;
    case 146:
        entry->Message = String(F("Rapid grid frequency change"));
        break;
    case 147:
        entry->Message = String(F("Power grid outage"));
        break;
    case 148:
        entry->Message = String(F("Grid disconnection"));
        break;
    case 149:
        entry->Message = String(F("Island detected"));
        break;
    case 205:
        entry->Message = String(F("Input port 1 & 2 overvoltage"));
        break;
    case 206:
        entry->Message = String(F("Input port 3 & 4 overvoltage"));
        break;
    case 207:
        entry->Message = String(F("Input port 1 & 2 undervoltage"));
        break;
    case 208:
        entry->Message = String(F("Input port 3 & 4 undervoltage"));
        break;
    case 209:
        entry->Message = String(F("Port 1 no input"));
        break;
    case 210:
        entry->Message = String(F("Port 2 no input"));
        break;
    case 211:
        entry->Message = String(F("Port 3 no input"));
        break;
    case 212:
        entry->Message = String(F("Port 4 no input"));
        break;
    case 213:
        entry->Message = String(F("PV-1 & PV-2 abnormal wiring"));
        break;
    case 214:
        entry->Message = String(F("PV-3 & PV-4 abnormal wiring"));
        break;
    case 215:
        entry->Message = String(F("PV-1 Input overvoltage"));
        break;
    case 216:
        entry->Message = String(F("PV-1 Input undervoltage"));
        break;
    case 217:
        entry->Message = String(F("PV-2 Input overvoltage"));
        break;
    case 218:
        entry->Message = String(F("PV-2 Input undervoltage"));
        break;
    case 219:
        entry->Message = String(F("PV-3 Input overvoltage"));
        break;
    case 220:
        entry->Message = String(F("PV-3 Input undervoltage"));
        break;
    case 221:
        entry->Message = String(F("PV-4 Input overvoltage"));
        break;
    case 222:
        entry->Message = String(F("PV-4 Input undervoltage"));
        break;
    case 301:
        entry->Message = String(F("Hardware error code 301"));
        break;
    case 302:
        entry->Message = String(F("Hardware error code 302"));
        break;
    case 303:
        entry->Message = String(F("Hardware error code 303"));
        break;
    case 304:
        entry->Message = String(F("Hardware error code 304"));
        break;
    case 305:
        entry->Message = String(F("Hardware error code 305"));
        break;
    case 306:
        entry->Message = String(F("Hardware error code 306"));
        break;
    case 307:
        entry->Message = String(F("Hardware error code 307"));
        break;
    case 308:
        entry->Message = String(F("Hardware error code 308"));
        break;
    case 309:
        entry->Message = String(F("Hardware error code 309"));
        break;
    case 310:
        entry->Message = String(F("Hardware error code 310"));
        break;
    case 311:
        entry->Message = String(F("Hardware error code 311"));
        break;
    case 312:
        entry->Message = String(F("Hardware error code 312"));
        break;
    case 313:
        entry->Message = String(F("Hardware error code 313"));
        break;
    case 314:
        entry->Message = String(F("Hardware error code 314"));
        break;
    case 5041:
        entry->Message = String(F("Error code-04 Port 1"));
        break;
    case 5042:
        entry->Message = String(F("Error code-04 Port 2"));
        break;
    case 5043:
        entry->Message = String(F("Error code-04 Port 3"));
        break;
    case 5044:
        entry->Message = String(F("Error code-04 Port 4"));
        break;
    case 5051:
        entry->Message = String(F("PV Input 1 Overvoltage/Undervoltage"));
        break;
    case 5052:
        entry->Message = String(F("PV Input 2 Overvoltage/Undervoltage"));
        break;
    case 5053:
        entry->Message = String(F("PV Input 3 Overvoltage/Undervoltage"));
        break;
    case 5054:
        entry->Message = String(F("PV Input 4 Overvoltage/Undervoltage"));
        break;
    case 5060:
        entry->Message = String(F("Abnormal bias"));
        break;
    case 5070:
        entry->Message = String(F("Over temperature protection"));
        break;
    case 5080:
        entry->Message = String(F("Grid Overvoltage/Undervoltage"));
        break;
    case 5090:
        entry->Message = String(F("Grid Overfrequency/Underfrequency"));
        break;
    case 5100:
        entry->Message = String(F("Island detected"));
        break;
    case 5120:
        entry->Message = String(F("EEPROM reading and writing error"));
        break;
    case 5150:
        entry->Message = String(F("10 min value grid overvoltage"));
        break;
    case 5200:
        entry->Message = String(F("Firmware error"));
        break;
    case 8310:
        entry->Message = String(F("Shut down"));
        break;
    case 9000:
        entry->Message = String(F("Microinverter is suspected of being stolen"));
        break;
    default:
        entry->Message = String(F("Unknown"));
        break;
    }
}

int AlarmLogParser::getTimezoneOffset()
{
    // see: https://stackoverflow.com/questions/13804095/get-the-time-zone-gmt-offset-in-c/44063597#44063597

    time_t gmt, rawtime = time(NULL);
    struct tm *ptm;

    struct tm gbuf;
    ptm = gmtime_r(&rawtime, &gbuf);

    // Request that mktime() looksup dst in timezone database
    ptm->tm_isdst = -1;
    gmt = mktime(ptm);

    return (int)difftime(rawtime, gmt);
}