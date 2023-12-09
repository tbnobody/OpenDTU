// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "Parser.h"

#define GRID_PROFILE_SIZE 141
#define PROFILE_TYPE_COUNT 7

typedef struct {
    uint8_t lIdx;
    uint8_t hIdx;
    const char* Name;
} ProfileType_t;

class GridProfileParser : public Parser {
public:
    GridProfileParser();
    void clearBuffer();
    void appendFragment(uint8_t offset, uint8_t* payload, uint8_t len);

    String getProfileName();
    String getProfileVersion();

    std::vector<uint8_t> getRawData();

private:
    uint8_t _payloadGridProfile[GRID_PROFILE_SIZE] = {};
    uint8_t _gridProfileLength = 0;

    static const std::array<const ProfileType_t, PROFILE_TYPE_COUNT> _profileTypes;
};