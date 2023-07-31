// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "Parser.h"
#include <Arduino.h>
#include <array>
#include <cstdint>

#define ALARM_LOG_ENTRY_COUNT 15
#define ALARM_LOG_ENTRY_SIZE 12
#define ALARM_LOG_PAYLOAD_SIZE (ALARM_LOG_ENTRY_COUNT * ALARM_LOG_ENTRY_SIZE + 4)

#define ALARM_MSG_COUNT 77

struct AlarmLogEntry_t {
    uint16_t MessageId;
    String Message;
    time_t StartTime;
    time_t EndTime;
};

enum class AlarmMessageType_t {
    ALL = 0,
    HMT
};

typedef struct {
    AlarmMessageType_t InverterType;
    uint16_t MessageId;
    String Message;
} AlarmMessage_t;

class AlarmLogParser : public Parser {
public:
    AlarmLogParser();
    void clearBuffer();
    void appendFragment(uint8_t offset, uint8_t* payload, uint8_t len);
    void beginAppendFragment();
    void endAppendFragment();

    uint8_t getEntryCount();
    void getLogEntry(uint8_t entryId, AlarmLogEntry_t* entry);

    void setLastAlarmRequestSuccess(LastCommandSuccess status);
    LastCommandSuccess getLastAlarmRequestSuccess();

    void setMessageType(AlarmMessageType_t type);

private:
    static int getTimezoneOffset();

    uint8_t _payloadAlarmLog[ALARM_LOG_PAYLOAD_SIZE];
    uint8_t _alarmLogLength = 0;

    LastCommandSuccess _lastAlarmRequestSuccess = CMD_NOK; // Set to NOK to fetch at startup

    AlarmMessageType_t _messageType = AlarmMessageType_t::ALL;

    static const std::array<const AlarmMessage_t, ALARM_MSG_COUNT> _alarmMessages;

    SemaphoreHandle_t _xSemaphore;
};