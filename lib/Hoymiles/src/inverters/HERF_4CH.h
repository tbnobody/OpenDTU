// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "HM_4CH.h"

class HERF_4CH : public HM_4CH {
public:
    explicit HERF_4CH(HoymilesRadio* radio, const uint64_t serial);
    static bool isValidSerial(const uint64_t serial);
    String typeName() const;
};
