// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Arduino.h"
#include "commands/CommandAbstract.h"
#include "queue/CommandQueue.h"
#include "types.h"
#include <TimeoutHelper.h>

#ifdef HOY_DEBUG_QUEUE
#define DEBUG_PRINT(fmt, args...) Serial.printf(fmt, ##args)
#else
#define DEBUG_PRINT(fmt, args...) /* Don't do anything in release builds */
#endif

class HoymilesRadio {
public:
    serial_u DtuSerial() const;
    virtual void setDtuSerial(const uint64_t serial);

    bool isIdle() const;
    bool isQueueEmpty() const;
    uint32_t getQueueSize() const;
    bool isInitialized() const;

    void removeCommands(InverterAbstract* inv);
    uint8_t countSimilarCommands(std::shared_ptr<CommandAbstract> cmd);

    void enqueCommand(std::shared_ptr<CommandAbstract> cmd)
    {
        DEBUG_PRINT("Queue size before: %ld\r\n", _commandQueue.size());
        DEBUG_PRINT("Handling command %s with type %d\r\n", cmd.get()->getCommandName().c_str(), static_cast<uint8_t>(cmd.get()->getQueueInsertType()));
        switch (cmd.get()->getQueueInsertType()) {
        case QueueInsertType::RemoveOldest:
            _commandQueue.removeDuplicatedEntries(cmd);
            break;
        case QueueInsertType::ReplaceExistent:
            // Checks if the queue already contains a command like the new one
            // and replaces the existing one with the new one.
            // (The new one will not be pushed at the end of the queue)
            if (_commandQueue.countSimilarCommands(cmd) > 0) {
                DEBUG_PRINT("    ... existing entry will be replaced\r\n");
                _commandQueue.replaceEntries(cmd);
                return;
            }
            break;
        case QueueInsertType::RemoveNewest:
            // Checks if the queue already contains a command like the new one
            // and drops the new one. The new one will not be inserted.
            if (_commandQueue.countSimilarCommands(cmd) > 0) {
                DEBUG_PRINT("    ... new entry will be dropped\r\n");
                return;
            }
            break;
        case QueueInsertType::AllowMultiple:
            // Dont do anything, just fall through and insert the command.
            break;
        }

        // Push the command into the queue if we reach this position of the code
        DEBUG_PRINT("    ... new entry will be appended\r\n");
        _commandQueue.push(cmd);

        DEBUG_PRINT("Queue size after: %ld\r\n", _commandQueue.size());
    }

    template <typename T>
    std::shared_ptr<T> prepareCommand(InverterAbstract* inv)
    {
        return std::make_shared<T>(inv);
    }

protected:
    static serial_u convertSerialToRadioId(const serial_u serial);
    static void dumpBuf(const uint8_t buf[], const uint8_t len, const bool appendNewline = true);

    bool checkFragmentCrc(const fragment_t& fragment) const;
    virtual void sendEsbPacket(CommandAbstract& cmd) = 0;
    void sendRetransmitPacket(const uint8_t fragment_id);
    void sendLastPacketAgain();
    void handleReceivedPackage();

    serial_u _dtuSerial;
    CommandQueue _commandQueue;
    bool _isInitialized = false;
    bool _busyFlag = false;

    TimeoutHelper _rxTimeout;
};
