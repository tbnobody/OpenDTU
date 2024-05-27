// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "Parser.h"
#include <array>
#include <cstdint>

#define ALARM_LOG_ENTRY_COUNT 15
#define ALARM_LOG_ENTRY_SIZE 12
#define ALARM_LOG_PAYLOAD_SIZE (ALARM_LOG_ENTRY_COUNT * ALARM_LOG_ENTRY_SIZE + 4)

#define ALARM_MSG_COUNT 131

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

enum class AlarmMessageLocale_t {
    EN,
    DE,
    FR
};

typedef struct {
    AlarmMessageType_t InverterType;
    uint16_t MessageId;
    const char* Message_en;
    const char* Message_de;
    const char* Message_fr;
} AlarmMessage_t;

class AlarmLogParser : public Parser {
public:
    AlarmLogParser();
    void clearBuffer();
    void appendFragment(const uint8_t offset, const uint8_t* payload, const uint8_t len);

    uint8_t getEntryCount() const;
    void getLogEntry(const uint8_t entryId, AlarmLogEntry_t& entry, const AlarmMessageLocale_t locale = AlarmMessageLocale_t::EN);

    void setLastAlarmRequestSuccess(const LastCommandSuccess status);
    LastCommandSuccess getLastAlarmRequestSuccess() const;

    void setMessageType(const AlarmMessageType_t type);

private:
    static int getTimezoneOffset();
    String getLocaleMessage(const AlarmMessage_t* msg, const AlarmMessageLocale_t locale) const;

    uint8_t _payloadAlarmLog[ALARM_LOG_PAYLOAD_SIZE];
    uint8_t _alarmLogLength = 0;

    LastCommandSuccess _lastAlarmRequestSuccess = CMD_NOK; // Set to NOK to fetch at startup

    AlarmMessageType_t _messageType = AlarmMessageType_t::ALL;

    static const std::array<const AlarmMessage_t, ALARM_MSG_COUNT> _alarmMessages;
};
