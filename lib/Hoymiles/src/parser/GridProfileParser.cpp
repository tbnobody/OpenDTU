// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 - 2024 Thomas Basler and others
 */

/*
This parser is used to parse the response of 'GridOnProFilePara'.
It contains the whole grid profile of the inverter.

Data structure:

00   01 02 03 04   05 06 07 08   09   10 11        12 13             14           15                16 17   18 19   20 21   22 23   24 25   26   27 28 29 30 31
                                      00 01        02 03             04           05                06 07   08 09   10 11   12 13
                                                                    |<---------- Returns till the end of the payload ---------->|
---------------------------------------------------------------------------------------------------------------------------------------------------------------
95   80 14 82 66   80 14 33 28   01   0A 00        20 01             00           0C                08 FC   07 A3   00 0F   09 E2   00 1E   E6   -- -- -- -- --
^^   ^^^^^^^^^^^   ^^^^^^^^^^^   ^^   ^^^^^        ^^^^^             ^^           ^^                ^^^^^   ^^^^^   ^^^^^   ^^^^^   ^^^^^   ^^
ID   Source Addr   Target Addr   Idx  Profile ID   Profile Version   Section ID   Section Version   Value   Value   Value   Value   CRC16   CRC8

The number of values depends on the respective section and its version. After the last value of a section follows the next section id.
*/
#include "GridProfileParser.h"
#include "../Hoymiles.h"
#include <cstring>
#include <frozen/map.h>
#include <frozen/string.h>

const std::array<const ProfileType_t, PROFILE_TYPE_COUNT> GridProfileParser::_profileTypes = { {
    { 0x02, 0x00, "US - NA_IEEE1547_240V" },
    { 0x03, 0x00, "DE - DE_VDE4105_2018" },
    { 0x03, 0x01, "DE - DE_VDE4105_2011" },
    { 0x0a, 0x00, "XX - EN 50549-1:2019" },
    { 0x0c, 0x00, "AT - AT_TOR_Erzeuger_default" },
    { 0x0d, 0x04, "XX - NF_EN_50549-1:2019" },
    { 0x10, 0x00, "ES - ES_RD1699" },
    { 0x12, 0x00, "PL - EU_EN50438" },
    { 0x29, 0x00, "NL - NL_NEN-EN50549-1_2019" },
    { 0x37, 0x00, "CH - CH_NA EEA-NE7-CH2020" },
} };

constexpr frozen::map<uint8_t, frozen::string, 12> profileSection = {
    { 0x00, "Voltage (H/LVRT)" },
    { 0x10, "Frequency (H/LFRT)" },
    { 0x20, "Island Detection (ID)" },
    { 0x30, "Reconnection (RT)" },
    { 0x40, "Ramp Rates (RR)" },
    { 0x50, "Frequency Watt (FW)" },
    { 0x60, "Volt Watt (VW)" },
    { 0x70, "Active Power Control (APC)" },
    { 0x80, "Volt Var (VV)" },
    { 0x90, "Specified Power Factor (SPF)" },
    { 0xA0, "Reactive Power Control (RPC)" },
    { 0xB0, "Watt Power Factor (WPF)" },
};

struct GridProfileItemDefinition_t {
    frozen::string Name;
    frozen::string Unit;
    uint8_t Divider;
};

constexpr GridProfileItemDefinition_t make_value(frozen::string Name, frozen::string Unit, uint8_t divisor)
{
    GridProfileItemDefinition_t v = { Name, Unit, divisor };
    return v;
}

constexpr frozen::map<uint8_t, GridProfileItemDefinition_t, 0x42> itemDefinitions = {
    { 0x01, make_value("Nominale Voltage (NV)", "V", 10) },
    { 0x02, make_value("Low Voltage 1 (LV1)", "V", 10) },
    { 0x03, make_value("LV1 Maximum Trip Time (MTT)", "s", 10) },
    { 0x04, make_value("High Voltage 1 (HV1)", "V", 10) },
    { 0x05, make_value("HV1 Maximum Trip Time (MTT)", "s", 10) },
    { 0x06, make_value("Low Voltage 2 (LV2)", "V", 10) },
    { 0x07, make_value("LV2 Maximum Trip Time (MTT)", "s", 100) },
    { 0x08, make_value("High Voltage 2 (HV2)", "V", 10) },
    { 0x09, make_value("HV2 Maximum Trip Time (MTT)", "s", 100) },
    { 0x0a, make_value("10mins Average High Voltage (AHV)", "V", 10) },
    { 0x0b, make_value("High Voltage 3 (HV3)", "V", 10) },
    { 0x0c, make_value("HV3 Maximum Trip Time (MTT)", "s", 100) },
    { 0x0d, make_value("Nominal Frequency", "Hz", 100) },
    { 0x0e, make_value("Low Frequency 1 (LF1)", "Hz", 100) },
    { 0x0f, make_value("LF1 Maximum Trip Time (MTT)", "s", 10) },
    { 0x10, make_value("High Frequency 1 (HF1)", "Hz", 100) },
    { 0x11, make_value("HF1 Maximum Trip time (MTT)", "s", 10) },
    { 0x12, make_value("Low Frequency 2 (LF2)", "Hz", 100) },
    { 0x13, make_value("LF2 Maximum Trip Time (MTT)", "s", 10) },
    { 0x14, make_value("High Frequency 2 (HF2)", "Hz", 100) },
    { 0x15, make_value("HF2 Maximum Trip time (MTT)", "s", 10) },
    { 0x16, make_value("ID Function Activated", "bool", 1) },
    { 0x17, make_value("Reconnect Time (RT)", "s", 10) },
    { 0x18, make_value("Reconnect High Voltage (RHV)", "V", 10) },
    { 0x19, make_value("Reconnect Low Voltage (RLV)", "V", 10) },
    { 0x1a, make_value("Reconnect High Frequency (RHF)", "Hz", 100) },
    { 0x1b, make_value("Reconnect Low Frequency (RLF)", "Hz", 100) },
    { 0x1c, make_value("Normal Ramp up Rate(RUR_NM)", "Rated%/s", 100) },
    { 0x1d, make_value("Soft Start Ramp up Rate (RUR_SS)", "Rated%/s", 100) },
    { 0x1e, make_value("FW Function Activated", "bool", 1) },
    { 0x1f, make_value("Start of Frequency Watt Droop (Fstart)", "Hz", 100) },
    { 0x20, make_value("FW Droop Slope (Kpower_Freq)", "Pn%/Hz", 10) },
    { 0x21, make_value("Recovery Ramp Rate (RRR)", "Pn%/s", 100) },
    { 0x22, make_value("Recovery High Frequency (RVHF)", "Hz", 10) },
    { 0x23, make_value("Recovery Low Frequency (RVLF)", "Hz", 100) },
    { 0x24, make_value("VW Function Activated", "bool", 1) },
    { 0x25, make_value("Start of Voltage Watt Droop (Vstart)", "V", 10) },
    { 0x26, make_value("End of Voltage Watt Droop (Vend)", "V", 10) },
    { 0x27, make_value("Droop Slope (Kpower_Volt)", "Pn%/V", 100) },
    { 0x28, make_value("APC Function Activated", "bool", 1) },
    { 0x29, make_value("Power Ramp Rate (PRR)", "Pn%/s", 100) },
    { 0x2a, make_value("VV Function Activated", "bool", 1) },
    { 0x2b, make_value("Voltage Set Point V1", "V", 10) },
    { 0x2c, make_value("Reactive Set Point Q1", "%Pn", 10) },
    { 0x2d, make_value("Voltage Set Point V2", "V", 10) },
    { 0x2e, make_value("Voltage Set Point V3", "V", 10) },
    { 0x2f, make_value("Voltage Set Point V4", "V", 10) },
    { 0x30, make_value("Reactive Set Point Q4", "%Pn", 10) },
    { 0x31, make_value("VV Setting Time (Tr)", "s", 10) },
    { 0x32, make_value("SPF Function Activated", "bool", 1) },
    { 0x33, make_value("Power Factor (PF)", "", 100) },
    { 0x34, make_value("RPC Function Activated", "bool", 1) },
    { 0x35, make_value("Reactive Power (VAR)", "%Sn", 1) },
    { 0x36, make_value("WPF Function Activated", "bool", 1) },
    { 0x37, make_value("Start of Power of WPF (Pstart)", "%Pn", 10) },
    { 0x38, make_value("Power Factor ar Rated Power (PFRP)", "", 100) },
    { 0x39, make_value("Low Voltage 3 (LV3)", "V", 10) },
    { 0x3a, make_value("LV3 Maximum Trip Time (MTT)", "s", 100) },
    { 0x3b, make_value("Momentary Cessition Low Voltage", "V", 10) },
    { 0x3c, make_value("Momentary Cessition High Voltage", "V", 10) },
    { 0x3d, make_value("FW Settling Time (Tr)", "s", 10) },
    { 0x3e, make_value("LF2 Maximum Trip Time (MTT)", "s", 100) },
    { 0x3f, make_value("HF2 Maximum Trip time (MTT)", "s", 100) },
    { 0x40, make_value("Short Interruption Reconnect Time (SRT)", "s", 10) },
    { 0x41, make_value("Short Interruption Time (SIT)", "s", 10) },
    { 0xff, make_value("Unkown Value", "", 1) },
};

const std::array<const GridProfileValue_t, SECTION_VALUE_COUNT> GridProfileParser::_profileValues = { {
    // Voltage (H/LVRT)
    // Version 0x00
    { 0x00, 0x00, 0x01 },
    { 0x00, 0x00, 0x02 },
    { 0x00, 0x00, 0x03 },
    { 0x00, 0x00, 0x04 },
    { 0x00, 0x00, 0x05 },

    // Version 0x01
    { 0x00, 0x01, 0x01 },
    { 0x00, 0x01, 0x02 },
    { 0x00, 0x01, 0x03 },
    { 0x00, 0x01, 0x04 },
    { 0x00, 0x01, 0x05 },
    { 0x00, 0x01, 0x08 },
    { 0x00, 0x01, 0x09 },

    // Version 0x02
    { 0x00, 0x02, 0x01 },
    { 0x00, 0x02, 0x02 },
    { 0x00, 0x02, 0x03 },
    { 0x00, 0x02, 0x04 },
    { 0x00, 0x02, 0x05 },
    { 0x00, 0x02, 0x06 },
    { 0x00, 0x02, 0x07 },

    // Version 0x03
    { 0x00, 0x03, 0x01 },
    { 0x00, 0x03, 0x02 },
    { 0x00, 0x03, 0x03 },
    { 0x00, 0x03, 0x05 },
    { 0x00, 0x03, 0x06 },
    { 0x00, 0x03, 0x07 },
    { 0x00, 0x03, 0x08 },
    { 0x00, 0x03, 0x09 },

    // Version 0x08
    { 0x00, 0x08, 0x01 },
    { 0x00, 0x08, 0x02 },
    { 0x00, 0x08, 0x03 },
    { 0x00, 0x08, 0x04 },
    { 0x00, 0x08, 0x05 },
    { 0x00, 0x08, 0xff },

    // Version 0x0a
    { 0x00, 0x0a, 0x01 },
    { 0x00, 0x0a, 0x02 },
    { 0x00, 0x0a, 0x03 },
    { 0x00, 0x0a, 0x04 },
    { 0x00, 0x0a, 0x05 },
    { 0x00, 0x0a, 0x06 },
    { 0x00, 0x0a, 0x07 },
    { 0x00, 0x0a, 0x0a },

    // Version 0x0b
    { 0x00, 0x0b, 0x01 },
    { 0x00, 0x0b, 0x02 },
    { 0x00, 0x0b, 0x03 },
    { 0x00, 0x0b, 0x04 },
    { 0x00, 0x0b, 0x05 },
    { 0x00, 0x0b, 0x06 },
    { 0x00, 0x0b, 0x07 },
    { 0x00, 0x0b, 0x08 },
    { 0x00, 0x0b, 0x09 },
    { 0x00, 0x0b, 0x0a },

    // Version 0x0c
    { 0x00, 0x0c, 0x01 },
    { 0x00, 0x0c, 0x02 },
    { 0x00, 0x0c, 0x03 },
    { 0x00, 0x0c, 0x04 },
    { 0x00, 0x0c, 0x05 },
    { 0x00, 0x0c, 0x06 },
    { 0x00, 0x0c, 0x07 },
    { 0x00, 0x0c, 0x08 },
    { 0x00, 0x0c, 0x09 },
    { 0x00, 0x0c, 0x0b },
    { 0x00, 0x0c, 0x0c },
    { 0x00, 0x0c, 0x0a },

    // Version 0x35
    { 0x00, 0x35, 0x01 },
    { 0x00, 0x35, 0x02 },
    { 0x00, 0x35, 0x03 },
    { 0x00, 0x35, 0x04 },
    { 0x00, 0x35, 0x05 },
    { 0x00, 0x35, 0x06 },
    { 0x00, 0x35, 0x07 },
    { 0x00, 0x35, 0x08 },
    { 0x00, 0x35, 0x09 },
    { 0x00, 0x35, 0x39 },
    { 0x00, 0x35, 0x3a },
    { 0x00, 0x35, 0x3b },
    { 0x00, 0x35, 0x3c },

    // Frequency (H/LFRT)
    // Version 0x00
    { 0x10, 0x00, 0x0d },
    { 0x10, 0x00, 0x0e },
    { 0x10, 0x00, 0x0f },
    { 0x10, 0x00, 0x10 },
    { 0x10, 0x00, 0x11 },

    // Version 0x03
    { 0x10, 0x03, 0x0d },
    { 0x10, 0x03, 0x0e },
    { 0x10, 0x03, 0x0f },
    { 0x10, 0x03, 0x10 },
    { 0x10, 0x03, 0x11 },
    { 0x10, 0x03, 0x12 },
    { 0x10, 0x03, 0x3e },
    { 0x10, 0x03, 0x14 },
    { 0x10, 0x03, 0x3f },

    // Island Detection (ID)
    // Version 0x00
    { 0x20, 0x00, 0x16 },

    // Reconnection (RT)
    // Version 0x03
    { 0x30, 0x03, 0x17 },
    { 0x30, 0x03, 0x18 },
    { 0x30, 0x03, 0x19 },
    { 0x30, 0x03, 0x1a },
    { 0x30, 0x03, 0x1b },

    // Version 0x07
    { 0x30, 0x07, 0x17 },
    { 0x30, 0x07, 0x18 },
    { 0x30, 0x07, 0x19 },
    { 0x30, 0x07, 0x1a },
    { 0x30, 0x07, 0x1b },
    { 0x30, 0x07, 0x40 },
    { 0x30, 0x07, 0x41 },

    // Ramp Rates (RR)
    // Version 0x00
    { 0x40, 0x00, 0x1c },
    { 0x40, 0x00, 0x1d },

    // Frequency Watt (FW)
    // Version 0x00
    { 0x50, 0x00, 0x1e },
    { 0x50, 0x00, 0x1f },
    { 0x50, 0x00, 0x20 },
    { 0x50, 0x00, 0x21 },

    // Version 0x01
    { 0x50, 0x01, 0x1e },
    { 0x50, 0x01, 0x1f },
    { 0x50, 0x01, 0x20 },
    { 0x50, 0x01, 0x21 },
    { 0x50, 0x01, 0x22 },

    // Version 0x08
    { 0x50, 0x08, 0x1e },
    { 0x50, 0x08, 0x1f },
    { 0x50, 0x08, 0x20 },
    { 0x50, 0x08, 0x21 },
    { 0x50, 0x08, 0x22 },
    { 0x50, 0x08, 0x23 },

    // Version 0x11
    { 0x50, 0x11, 0x1e },
    { 0x50, 0x11, 0x1f },
    { 0x50, 0x11, 0x20 },
    { 0x50, 0x11, 0x21 },
    { 0x50, 0x11, 0x3d },

    // Volt Watt (VW)
    // Version 0x00
    { 0x60, 0x00, 0x24 },
    { 0x60, 0x00, 0x25 },
    { 0x60, 0x00, 0x26 },
    { 0x60, 0x00, 0x27 },

    // Version 0x04
    { 0x60, 0x04, 0x24 },
    { 0x60, 0x04, 0x25 },
    { 0x60, 0x04, 0x26 },
    { 0x60, 0x04, 0x27 },

    // Active Power Control (APC)
    // Version 0x00
    { 0x70, 0x00, 0x28 },

    // Version 0x02
    { 0x70, 0x02, 0x28 },
    { 0x70, 0x02, 0x29 },

    // Volt Var (VV)
    // Version 0x00
    { 0x80, 0x00, 0x2a },
    { 0x80, 0x00, 0x2b },
    { 0x80, 0x00, 0x2c },
    { 0x80, 0x00, 0x2d },
    { 0x80, 0x00, 0x2e },
    { 0x80, 0x00, 0x2f },
    { 0x80, 0x00, 0x30 },

    // Version 0x01
    { 0x80, 0x01, 0x2a },
    { 0x80, 0x01, 0x2b },
    { 0x80, 0x01, 0x2c },
    { 0x80, 0x01, 0x2d },
    { 0x80, 0x01, 0x2e },
    { 0x80, 0x01, 0x2f },
    { 0x80, 0x01, 0x30 },
    { 0x80, 0x01, 0x31 },

    // Specified Power Factor (SPF)
    // Version 0x00
    { 0x90, 0x00, 0x32 },
    { 0x90, 0x00, 0x33 },

    // Reactive Power Control (RPC)
    // Version 0x02
    { 0xa0, 0x02, 0x34 },
    { 0xa0, 0x02, 0x35 },

    // Watt Power Factor (WPF)
    // Version 0x00
    { 0xb0, 0x00, 0x36 },
    { 0xb0, 0x00, 0x37 },
    { 0xb0, 0x00, 0x38 },
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

void GridProfileParser::appendFragment(const uint8_t offset, const uint8_t* payload, const uint8_t len)
{
    if (offset + len > GRID_PROFILE_SIZE) {
        Hoymiles.getMessageOutput()->printf("FATAL: (%s, %d) grid profile packet too large for buffer\r\n", __FILE__, __LINE__);
        return;
    }
    memcpy(&_payloadGridProfile[offset], payload, len);
    _gridProfileLength += len;
}

String GridProfileParser::getProfileName() const
{
    for (auto& ptype : _profileTypes) {
        if (ptype.lIdx == _payloadGridProfile[0] && ptype.hIdx == _payloadGridProfile[1]) {
            return ptype.Name;
        }
    }
    return "Unknown";
}

String GridProfileParser::getProfileVersion() const
{
    char buffer[10];
    HOY_SEMAPHORE_TAKE();
    snprintf(buffer, sizeof(buffer), "%d.%d.%d", (_payloadGridProfile[2] >> 4) & 0x0f, _payloadGridProfile[2] & 0x0f, _payloadGridProfile[3]);
    HOY_SEMAPHORE_GIVE();
    return buffer;
}

std::vector<uint8_t> GridProfileParser::getRawData() const
{
    std::vector<uint8_t> ret;
    HOY_SEMAPHORE_TAKE();
    for (uint8_t i = 0; i < _gridProfileLength; i++) {
        ret.push_back(_payloadGridProfile[i]);
    }
    HOY_SEMAPHORE_GIVE();
    return ret;
}

std::list<GridProfileSection_t> GridProfileParser::getProfile() const
{
    std::list<GridProfileSection_t> l;

    if (_gridProfileLength > 4) {
        uint16_t pos = 4;
        do {
            const uint8_t section_id = _payloadGridProfile[pos];
            const uint8_t section_version = _payloadGridProfile[pos + 1];
            const int16_t section_start = getSectionStart(section_id, section_version);
            const uint8_t section_size = getSectionSize(section_id, section_version);
            pos += 2;

            GridProfileSection_t section;
            try {
                section.SectionName = profileSection.at(section_id).data();
            } catch (const std::out_of_range&) {
                section.SectionName = "Unknown";
                break;
            }

            if (section_start == -1) {
                section.SectionName = "Unknown";
                break;
            }

            for (uint8_t val_id = 0; val_id < section_size; val_id++) {
                auto itemDefinition = itemDefinitions.at(_profileValues[section_start + val_id].ItemDefinition);

                float value = static_cast<int16_t>((_payloadGridProfile[pos] << 8) | _payloadGridProfile[pos + 1]);
                value /= itemDefinition.Divider;

                GridProfileItem_t v;
                v.Name = itemDefinition.Name.data();
                v.Unit = itemDefinition.Unit.data();
                v.Value = value;
                section.items.push_back(v);

                pos += 2;
            }

            l.push_back(section);

        } while (pos < _gridProfileLength);
    }

    return l;
}

bool GridProfileParser::containsValidData() const
{
    return _gridProfileLength > 6;
}

uint8_t GridProfileParser::getSectionSize(const uint8_t section_id, const uint8_t section_version)
{
    uint8_t count = 0;
    for (auto& values : _profileValues) {
        if (values.Section == section_id && values.Version == section_version) {
            count++;
        }
    }
    return count;
}

int16_t GridProfileParser::getSectionStart(const uint8_t section_id, const uint8_t section_version)
{
    int16_t count = -1;
    for (auto& values : _profileValues) {
        count++;
        if (values.Section == section_id && values.Version == section_version) {
            break;
        }
    }
    return count;
}
