// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "Parser.h"
#include <list>

#define GRID_PROFILE_SIZE 141
#define PROFILE_TYPE_COUNT 7
#define SECTION_VALUE_COUNT 113

typedef struct {
    uint8_t lIdx;
    uint8_t hIdx;
    const char* Name;
} ProfileType_t;

struct GridProfileValue_t {
    uint8_t Section;
    uint8_t Version;
    uint8_t Type;
};

struct GridProfileItem_t {
    String Name;
    String Unit;
    float Value;
};

struct GridProfileSection_t {
    String SectionName;
    std::list<GridProfileItem_t> items;
};

class GridProfileParser : public Parser {
public:
    GridProfileParser();
    void clearBuffer();
    void appendFragment(uint8_t offset, uint8_t* payload, uint8_t len);

    String getProfileName();
    String getProfileVersion();

    std::vector<uint8_t> getRawData();

    std::list<GridProfileSection_t> getProfile();

private:
    static uint8_t getSectionSize(uint8_t section_id, uint8_t section_version);
    static int8_t getSectionStart(uint8_t section_id, uint8_t section_version);

    uint8_t _payloadGridProfile[GRID_PROFILE_SIZE] = {};
    uint8_t _gridProfileLength = 0;

    static const std::array<const ProfileType_t, PROFILE_TYPE_COUNT> _profileTypes;
    static const std::array<const GridProfileValue_t, SECTION_VALUE_COUNT> _profile_values;
};