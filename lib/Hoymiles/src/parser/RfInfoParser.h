// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "Parser.h"
#include <cstddef>
#include <cstdint>

enum class RfModuleLine : uint8_t {
    Unknown,
    A,
    B,
    C,
    D
};

class RfInfoParser : public Parser {
public:
    // Parses a complete (single fragment) answer and stores the result.
    // Returns false if the answer is too short to be parsed.
    [[nodiscard]] bool setPayload(const uint8_t* payload, const uint8_t len);

    String getRfHardwareVersionStr() const;
    String getRfFirmwareVersionStr() const;

    uint32_t getRfFirmwareVersionRaw() const;

    RfModuleLine getModuleLine() const;
    char getModuleLineChar() const;

    bool hasModuleStatus() const;
    uint8_t getModuleStatus() const;

    bool containsValidData() const;

    static RfModuleLine lineFromVersion(const uint32_t rfSw);

private:
    static constexpr size_t RF_INFO_SIZE = 14;
    static constexpr size_t RF_HW_VERSION_OFFSET = 6;
    static constexpr size_t RF_FW_VERSION_OFFSET = 10;
    static constexpr size_t RF_STATUS_OFFSET = 14;
    static constexpr uint8_t RF_STATUS_ENCRYPTED = 0x01;

    static String versionStr(const uint8_t* payload, const size_t offset);

    String _rfHwVersion;
    String _rfFwVersion;
    uint32_t _rfFwRaw = 0;
    RfModuleLine _line = RfModuleLine::Unknown;
    bool _hasStatus = false;
    uint8_t _status = 0;
};
