// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "Parser.h"

#define GRID_PROFILE_SIZE 141

class GridProfileParser : public Parser {
public:
    GridProfileParser();
    void clearBuffer();
    void appendFragment(uint8_t offset, uint8_t* payload, uint8_t len);

    std::vector<uint8_t> getRawData();

private:
    uint8_t _payloadGridProfile[GRID_PROFILE_SIZE] = {};
    uint8_t _gridProfileLength = 0;
};