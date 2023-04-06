// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "HM_Abstract.h"

class HMS_Abstract : public HM_Abstract {
public:
    explicit HMS_Abstract(HoymilesRadio* radio, uint64_t serial);

    virtual bool sendChangeChannelRequest();
};