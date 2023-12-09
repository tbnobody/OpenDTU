// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "GridProfileParser.h"
#include "../Hoymiles.h"
#include <cstring>

const std::array<const ProfileType_t, PROFILE_TYPE_COUNT> GridProfileParser::_profileTypes = { {
    { 0x02, 0x00, "no data (yet)" },
    { 0x03, 0x00, "Germany - DE_VDE4105_2018" },
    { 0x0a, 0x00, "European - EN 50549-1:2019" },
    { 0x0c, 0x00, "AT Tor - EU_EN50438" },
    { 0x0d, 0x04, "France" },
    { 0x12, 0x00, "Poland" },
    { 0x37, 0x00, "Swiss - CH_NA EEA-NE7-CH2020" },
} };

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

String GridProfileParser::getProfileName()
{
    for (auto& ptype : _profileTypes) {
        if (ptype.lIdx == _payloadGridProfile[0] && ptype.hIdx == _payloadGridProfile[1]) {
            return ptype.Name;
        }
    }
    return "Unknown";
}

String GridProfileParser::getProfileVersion()
{
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%d.%d.%d", (_payloadGridProfile[2] >> 4) & 0x0f, _payloadGridProfile[2] & 0x0f, _payloadGridProfile[3]);
    return buffer;
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
