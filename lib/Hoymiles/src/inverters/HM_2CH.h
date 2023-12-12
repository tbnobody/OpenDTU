// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "HM_Abstract.h"

class HM_2CH : public HM_Abstract {
public:
    explicit HM_2CH(HoymilesRadio* radio, const uint64_t serial);
    static bool isValidSerial(const uint64_t serial);
    String typeName() const;
    const byteAssign_t* getByteAssignment() const;
    uint8_t getByteAssignmentSize() const;
};