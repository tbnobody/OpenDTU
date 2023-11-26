// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "Parser.h"
#include <map>
#include <vector>

#define GRID_PROFILE_SIZE 141

class GridProfileParser : public Parser {
public:
    GridProfileParser();
    void clearBuffer();
    void appendFragment(uint8_t offset, uint8_t* payload, uint8_t len);

    std::vector<uint8_t> getRawData();
    int GridProfileParser::modbusCrc(std::string msg);
    std::string parseGridProfile(std::string hex_string);

private:
    uint8_t _payloadGridProfile[GRID_PROFILE_SIZE] = {};
    uint8_t _gridProfileLength = 0;

    std::map<int, std::map<int, std::string>> profile_types = {
        {0x02, {{0x00, "no data (yet)"}}},
        {0x03, {{0x00, "Germany - DE_VDE4105_2018"}}},
        {0x0a, {{0x00, "European - EN 50549-1:2019"}}},
        {0x0c, {{0x00, "AT Tor - EU_EN50438"}}},
        {0x0d, {{0x04, "France"}}},
        {0x12, {{0x00, "Poland - EU_EN50438"}}},
        {0x37, {{0x00, "Swiss - CH_NA EEA-NE7-CH2020"}}}
    };

    std::map<int, std::string> profile_structs = {
        {0x00, "Voltage (H/LVRT)"},
        {0x10, "Frequency (H/LFRT)"},
        {0x20, "Island Detection (ID)"},
        {0x30, "Reconnection (RT)"},
        {0x40, "Ramp Rates (RR)"},
        {0x50, "Frequency Watt (FW)"},
        {0x60, "Volt Watt (VW)"},
        {0x70, "Active Power Control (APC)"},
        {0x80, "Volt Var (VV)"},
        {0x90, "Specified Power Factor (SPF)"},
        {0xA0, "Reactive Power Control (RPC)"},
        {0xB0, "Watt Power Factor (WPF)"}
    };

    std::map<int, std::map<int, std::vector<std::vector<std::string>>>> profile_details = {
        {0x00, {
            {0x0A, {
                {"Nominale Voltage (NV)", "V", "10"},
                {"Low Voltage 1 (LV1)", "V", "10"},
                {"LV1 Maximum Trip Time (MTT)", "s", "10"},
                {"High Voltage 1 (HV1)", "V", "10"},
                {"HV1 Maximum Trip Time (MTT)", "s", "10"},
                {"Low Voltage 2 (LV2)", "V", "10"},
                {"LV2 Maximum Trip Time (MTT)", "s", "10"},
                {"10mins Average High Voltage (AHV)", "V", "10"}
            }},
            {0x0B, {
                {"Nominale Voltage (NV)", "V", "10"},
                {"Low Voltage 1 (LV1)", "V", "10"},
                {"LV1 Maximum Trip Time (MTT)", "s", "10"},
                {"High Voltage 1 (HV1)", "V", "10"},
                {"HV1 Maximum Trip Time (MTT)", "s", "10"},
                {"Low Voltage 2 (LV2)", "V", "10"},
                {"LV2 Maximum Trip Time (MTT)", "s", "10"},
                {"High Voltage 2 (HV2)", "V", "10"},
                {"HV2 Maximum Trip Time (MTT)", "s", "10"},
                {"10mins Average High Voltage (AHV)", "V", "10"}
            }},
            {0x00, {
                {"Nominale Voltage (NV)", "V", "10"},
                {"Low Voltage 1 (LV1)", "V", "10"},
                {"LV1 Maximum Trip Time (MTT)", "s", "10"},
                {"High Voltage 1 (HV1)", "V", "10"},
                {"HV1 Maximum Trip Time (MTT)", "s", "10"}
            }},
            {0x03, {
                {"Nominale Voltage (NV)", "V", "10"},
                {"Low Voltage 1 (LV1)", "V", "10"},
                {"LV1 Maximum Trip Time (MTT)", "s", "10"},
                {"High Voltage 1 (HV1)", "V", "10"},
                {"HV1 Maximum Trip Time (MTT)", "s", "10"},
                {"Low Voltage 2 (LV2)", "V", "10"},
                {"LV2 Maximum Trip Time (MTT)", "s", "10"},
                {"High Voltage 2 (HV2)", "V", "10"},
                {"HV2 Maximum Trip Time (MTT)", "s", "10"}
            }},
            {0x0C, {
                {"Nominale Voltage (NV)", "V", "10"},
                {"Low Voltage 1 (LV1)", "V", "10"},
                {"LV1 Maximum Trip Time (MTT)", "s", "10"},
                {"High Voltage 1 (HV1)", "V", "10"},
                {"HV1 Maximum Trip Time (MTT)", "s", "10"},
                {"Low Voltage 2 (LV2)", "V", "10"},
                {"LV2 Maximum Trip Time (MTT)", "s", "10"},
                {"High Voltage 2 (HV2)", "V", "10"},
                {"HV2 Maximum Trip Time (MTT)", "s", "10"},
                {"High Voltage 3 (HV3)", "V", "10"},
                {"HV3 Maximum Trip Time (MTT)", "s", "10"},
                {"10mins Average High Voltage (AHV)", "V", "10"}
            }}
        }},
        {0x10, {
            {0x00, {
                {"Nominal Frequency", "Hz", "100"},
                {"Low Frequency 1 (LF1)", "Hz", "100"},
                {"LF1 Maximum Trip Time (MTT)", "s", "10"},
                {"High Frequency 1 (HF1)", "Hz", "100"},
                {"HF1 Maximum Trip time (MTT)", "s", "10"}
            }},
            {0x03, {
                {"Nominal Frequency", "Hz", "100"},
                {"Low Frequency 1 (LF1)", "Hz", "100"},
                {"LF1 Maximum Trip Time (MTT)", "s", "10"},
                {"High Frequency 1 (HF1)", "Hz", "100"},
                {"HF1 Maximum Trip time (MTT)", "s", "10"},
                {"Low Frequency 2 (LF2)", "Hz", "100"},
                {"LF2 Maximum Trip Time (MTT)", "s", "10"},
                {"High Frequency 2 (HF2)", "Hz", "100"},
                {"HF2 Maximum Trip time (MTT)", "s", "10"}
            }}
        }},
        {0x20, {
            {0x00, {
                {"ID Function Activated", "bool", "1"}
            }}
        }},
        {0x30, {
            {0x03, {
                {"Reconnect Time (RT)", "s", "10"},
                {"Reconnect High Voltage (RHV)", "V", "10"},
                {"Reconnect Low Voltage (RLV)", "V", "10"},
                {"Reconnect High Frequency (RHF)", "Hz", "100"},
                {"Reconnect Low Frequency (RLF)", "Hz", "100"}
            }}
        }},
        {0x40, {
            {0x00, {
                {"Normal Ramp up Rate(RUR_NM)", "Rated%/s", "100"},
                {"Soft Start Ramp up Rate (RUR_SS)", "Rated%/s", "100"}
            }}
        }},
        {0x50, {
            {0x08, {
                {"FW Function Activated", "bool", "1"},
                {"Start of Frequency Watt Droop (Fstart)", "Hz", "100"},
                {"FW Droop Slope (Kpower_Freq)", "Pn%/Hz", "10"},
                {"Recovery Ramp Rate (RRR)", "Pn%/s", "100"},
                {"Recovery High Frequency (RVHF)", "Hz", "100"}, // may need to be div 10
                {"Recovery Low Frequency (RVLF)", "Hz", "100"}
            }},
            {0x00, {
                {"FW Function Activated", "bool", "1"},
                {"Start of Frequency Watt Droop (Fstart)", "Hz", "100"},
                {"FW Droop Slope (Kpower_Freq)", "Pn%/Hz", "10"},
                {"Recovery Ramp Rate (RRR)", "Pn%/s", "100"}
            }},
            {0x01, {
                {"FW Function Activated", "bool", "1"},
                {"Start of Frequency Watt Droop (Fstart)", "Hz", "100"},
                {"FW Droop Slope (Kpower_Freq)", "Pn%/Hz", "10"},
                {"Recovery Ramp Rate (RRR)", "Pn%/s", "100"},
                {"Recovery High Frequency (RVHF)", "Hz", "100"} // may need to be div 10
            }}
        }},
        {0x60, {
            {0x00, {
                {"VW Function Activated", "bool", "1"},
                {"Start of Voltage Watt Droop (Vstart)", "V", "10"},
                {"End of Voltage Watt Droop (Vend)", "V", "10"},
                {"Droop Slope (Kpower_Volt)", "Pn%/V", "100"}
            }},
            {0x04, {
                {"VW Function Activated", "bool", "1"},
                {"Start of Voltage Watt Droop (Vstart)", "V", "10"},
                {"End of Voltage Watt Droop (Vend)", "V", "10"},
                {"Droop Slope (Kpower_Volt)", "Pn%/V", "100"}
            }}
        }},
        {0x70, {
            {0x02, {
                {"APC Function Activated", "bool", "1"},
                {"Power Ramp Rate (PRR)", "Pn%/s", "100"}
            }},
            {0x00, {
                {"APC Function Activated", "bool", "1"}
            }}
        }},
        {0x80, {
            {0x00, {
                {"VV Function Activated", "bool", "1"},
                {"Voltage Set Point V1", "V", "10"},
                {"Reactive Set Point Q1", "%Pn", "10"},
                {"Voltage Set Point V2", "V", "10"},
                {"Voltage Set Point V3", "V", "10"},
                {"Voltage Set Point V4", "V", "10"},
                {"Reactive Set Point Q4", "%Pn", "10"}
            }},
            {0x01, {
                {"VV Function Activated", "bool", "1"},
                {"Voltage Set Point V1", "V", "10"},
                {"Reactive Set Point Q1", "%Pn", "10"},
                {"Voltage Set Point V2", "V", "10"},
                {"Voltage Set Point V3", "V", "10"},
                {"Voltage Set Point V4", "V", "10"},
                {"Reactive Set Point Q4", "%Pn", "10"},
                {"Setting Time (Tr)", "s", "10"}
            }}
        }},
        {0x90, {
            {0x00, {
                {"SPF Function Activated", "bool", "1"},
                {"Power Factor (PF)", "", "100"}
            }}
        }},
        {0xA0, {
            {0x02, {
                {"RPC Function Activated", "bool", "1"},
                {"Reactive Power (VAR)", "%Sn", "1"}
            }}
        }},
        {0xB0, {
            {0x00, {
                {"WPF Function Activated", "bool", "1"},
                {"Start of Power of WPF (Pstart)", "%Pn", "10"},
                {"Power Factor ar Rated Power (PFRP)", "", "100"}
            }}
        }}
    };    
};