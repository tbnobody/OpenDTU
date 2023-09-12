// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "RealTimeRunDataCommand.h"
#include "Hoymiles.h"
#include "inverters/InverterAbstract.h"

RealTimeRunDataCommand::RealTimeRunDataCommand(uint64_t target_address, uint64_t router_address, time_t time)
    : MultiDataCommand(target_address, router_address)
{
    setTime(time);
    setDataType(0x0b);
    setTimeout(500);
}

String RealTimeRunDataCommand::getCommandName()
{
    return "RealTimeRunData";
}

bool RealTimeRunDataCommand::handleResponse(InverterAbstract* inverter, fragment_t fragment[], uint8_t max_fragment_id)
{
    // Check CRC of whole payload
    if (!MultiDataCommand::handleResponse(inverter, fragment, max_fragment_id)) {
        return false;
    }

    // Check if at least all required bytes are received
    // In case of low power in the inverter it occours that some incomplete fragments
    // with a valid CRC are received.
    uint8_t fragmentsSize = getTotalFragmentSize(fragment, max_fragment_id);
    uint8_t expectedSize = inverter->Statistics()->getExpectedByteCount();
    if (fragmentsSize < expectedSize) {
        Hoymiles.getMessageOutput()->printf("ERROR in %s: Received fragment size: %d, min expected size: %d\r\n",
            getCommandName().c_str(), fragmentsSize, expectedSize);

        return false;
    }

    // Move all fragments into target buffer
    uint8_t offs = 0;
    inverter->Statistics()->beginAppendFragment();
    inverter->Statistics()->clearBuffer();
    for (uint8_t i = 0; i < max_fragment_id; i++) {
        inverter->Statistics()->appendFragment(offs, fragment[i].fragment, fragment[i].len);
        offs += (fragment[i].len);
    }
    inverter->Statistics()->endAppendFragment();
    inverter->Statistics()->resetRxFailureCount();
    inverter->Statistics()->setLastUpdate(millis());
    return true;
}

void RealTimeRunDataCommand::gotTimeout(InverterAbstract* inverter)
{
    inverter->Statistics()->incrementRxFailureCount();

    if (inverter->getZeroValuesIfUnreachable() && !inverter->isReachable()) {
        Hoymiles.getMessageOutput()->println("Set runtime data to zero");
        inverter->Statistics()->zeroRuntimeData();
    }
}