// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "Parser.h"
#include <cstddef>

class RfInfoParser : public Parser {
public:
    // Parses a complete (single fragment) answer and stores the result.
    // Returns false if the answer is too short to be parsed.
    [[nodiscard]] bool setPayload(const uint8_t* payload, const uint8_t len);

    String getRfHardwareVersionStr() const;
    String getRfFirmwareVersionStr() const;

    bool containsValidData() const;

private:
    static constexpr size_t RF_INFO_SIZE = 14;
    static constexpr size_t RF_HW_VERSION_OFFSET = 6;
    static constexpr size_t RF_FW_VERSION_OFFSET = 10;

    static String versionStr(const uint8_t* payload, const size_t offset);

    String _rfHwVersion;
    String _rfFwVersion;
};
