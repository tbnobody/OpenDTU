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
};