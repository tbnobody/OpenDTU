// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "FrequencyManagerAbstract.h"

class FrequencyManager_NOOP:public FrequencyManagerAbstract {
public:
    explicit FrequencyManager_NOOP(InverterAbstract* inv);

    uint32_t getTXFrequency(CommandAbstract& cmd);
    uint32_t getRXFrequency(CommandAbstract& cmd);

    void processRXResult(CommandAbstract *cmd, uint8_t verify_fragments_result);

    bool shouldSendChangeChannelCommand();
    void startNextFetch();
};