// SPDX-License-Identifier: GPL-2.0-or-later

#include "FrequencyManagerAbstract.h"
#include "inverters/InverterAbstract.h"
#include "commands/CommandAbstract.h"

FrequencyManagerAbstract::FrequencyManagerAbstract(InverterAbstract* inv) {
    this->_inv = inv;
}