#pragma once
#include "Parser.h"
#include <cstdint>
#include <WString.h>
#include <time.h>
#include "Clock.h"

#define ALARM_LOG_ENTRY_COUNT 15
#define ALARM_LOG_ENTRY_SIZE 12

struct AlarmLogEntry_t {
    uint16_t MessageId;
    String Message;
    time_t StartTime;
    time_t EndTime;
};

class AlarmLogParser : public Parser {
public:
    AlarmLogParser(Clock* clock);
    void clearBuffer();
    void appendFragment(uint8_t offset, uint8_t* payload, uint8_t len);

    uint8_t getEntryCount();
    void getLogEntry(uint8_t entryId, AlarmLogEntry_t* entry);

private:
    Clock* _clock;

    uint8_t _payloadAlarmLog[ALARM_LOG_ENTRY_SIZE * ALARM_LOG_ENTRY_COUNT];
    uint8_t _alarmLogLength;
};