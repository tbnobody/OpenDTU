// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2024 Thomas Basler and others
 */

 #include "FrequencyManager_NOOP.h"
 #include "Hoymiles.h"
 #include "inverters/InverterAbstract.h"
 #include "commands/CommandAbstract.h"
 
 FrequencyManager_NOOP::FrequencyManager_NOOP(InverterAbstract* inv):FrequencyManagerAbstract(inv) {
 }
 
 uint32_t FrequencyManager_NOOP::getTXFrequency(CommandAbstract& cmd) {
    return Hoymiles.getRadioCmt()->getInverterTargetFrequency();
 }
 
 uint32_t FrequencyManager_NOOP::getRXFrequency(CommandAbstract& cmd) {
    return Hoymiles.getRadioCmt()->getInverterTargetFrequency();
 }

 void FrequencyManager_NOOP::processRXResult(CommandAbstract *cmd, uint8_t verify_fragments_result) {
     (void) cmd;
     (void) verify_fragments_result;
 }
 
 
 bool FrequencyManager_NOOP::shouldSendChangeChannelCommand() {
    return false;
 }
 
 void FrequencyManager_NOOP::startNextFetch() {
 }