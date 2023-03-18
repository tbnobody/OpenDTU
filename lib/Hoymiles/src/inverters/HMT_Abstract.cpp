// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "HMT_Abstract.h"
#include "parser/AlarmLogParser.h"

HMT_Abstract::HMT_Abstract(HoymilesRadio* radio, uint64_t serial)
    : HM_Abstract(radio, serial)
{
    EventLog()->setMessageType(AlarmMessageType_t::HMT);
};
