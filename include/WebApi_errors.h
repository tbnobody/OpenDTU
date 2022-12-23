// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

enum WebApiError {
    GenericBase = 1000,
    GenericSuccess,
    GenericNoValueFound,
    GenericDataTooLarge,
    GenericParseError,
    GenericValueMissing,

    DtuBase = 2000,
    DtuSerialZero,
    DtuPollZero,
    DtuInvalidPowerLevel,

    ConfigBase = 3000,
    ConfigNotDeleted,
    ConfigSuccess,

    InverterBase = 4000,
    InverterSerialZero,
    InverterNameLength,
    InverterCount,
    InverterAdded,
    InverterInvalidId,
    InverterInvalidMaxChannel,
    InverterChanged,
    InverterDeleted,

    LimitBase = 5000,
    LimitSerialZero,
    LimitInvalidLimit,
    LimitInvalidType,
    LimitInvalidInverter,
};