// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "HMT_Abstract.h"

class HMT_6CH : public HMT_Abstract {
public:
    explicit HMT_6CH(HoymilesRadio* radio, uint64_t serial);
    static bool isValidSerial(uint64_t serial);
    String typeName();
    const byteAssign_t* getByteAssignment();
    uint8_t getByteAssignmentSize();
};