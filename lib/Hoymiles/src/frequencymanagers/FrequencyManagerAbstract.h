// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Arduino.h"
#include "types.h"

class InverterAbstract;
class CommandAbstract;

class FrequencyManagerAbstract {
public:
    explicit FrequencyManagerAbstract(InverterAbstract* inv);
    virtual ~FrequencyManagerAbstract() {};

    virtual uint32_t getTXFrequency(CommandAbstract& cmd) = 0;
    virtual uint32_t getRXFrequency(CommandAbstract& cmd) = 0;

    virtual void processRXResult(CommandAbstract *cmd, uint8_t verify_fragments_result) = 0;
    virtual bool shouldSendChangeChannelCommand() = 0;
    virtual void startNextFetch() = 0;
protected:

    InverterAbstract* _inv;
};
