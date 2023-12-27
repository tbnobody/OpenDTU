// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "HMT_Abstract.h"

class HMT_4CH : public HMT_Abstract {
public:
    explicit HMT_4CH(HoymilesRadio* radio, const uint64_t serial);
    static bool isValidSerial(const uint64_t serial);
    String typeName() const;
    const byteAssign_t* getByteAssignment() const;
    uint8_t getByteAssignmentSize() const;
};