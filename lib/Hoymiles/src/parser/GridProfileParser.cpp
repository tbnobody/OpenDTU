// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "GridProfileParser.h"
#include "../Hoymiles.h"
#include <cstring>

GridProfileParser::GridProfileParser()
    : Parser()
{
    clearBuffer();
}

void GridProfileParser::clearBuffer()
{
    memset(_payloadGridProfile, 0, GRID_PROFILE_SIZE);
    _gridProfileLength = 0;
}

void GridProfileParser::appendFragment(uint8_t offset, uint8_t* payload, uint8_t len)
{
    if (offset + len > GRID_PROFILE_SIZE) {
        Hoymiles.getMessageOutput()->printf("FATAL: (%s, %d) grid profile packet too large for buffer\r\n", __FILE__, __LINE__);
        return;
    }
    memcpy(&_payloadGridProfile[offset], payload, len);
    _gridProfileLength += len;
}

std::vector<uint8_t> GridProfileParser::getRawData()
{
    std::vector<uint8_t> ret;
    HOY_SEMAPHORE_TAKE();
    for (uint8_t i = 0; i < GRID_PROFILE_SIZE; i++) {
        ret.push_back(_payloadGridProfile[i]);
    }
    HOY_SEMAPHORE_GIVE();
    return ret;
}
