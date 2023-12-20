// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "HMS_Abstract.h"
#include <list>

class HMS_1CH : public HMS_Abstract {
public:
    explicit HMS_1CH(HoymilesRadio* radio, const uint64_t serial);
    static bool isValidSerial(const uint64_t serial);
    String typeName() const;
    const byteAssign_t* getByteAssignment() const;
    uint8_t getByteAssignmentSize() const;
};