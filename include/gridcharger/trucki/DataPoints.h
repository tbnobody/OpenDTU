#pragma once

#include <DataPoints.h>
#include <string>

namespace GridChargers::Trucki {

enum class DataPointLabel : uint8_t {
    ZEPC,
    State,
    BatteryGridState,
    Temperature,
    Efficiency,
    DayEnergy,
    TotalEnergy,
    AcVoltage,
    MaxAcPower,
    MinAcPower,
    AcPowerSetpoint,
    AcPower,
    DcVoltage,
    DcPower,
    DcVoltageSetpoint,
    DcCurrent,
    DcVoltageOffline,
    DcCurrentOffline,
};

template<DataPointLabel> struct DataPointLabelTraits;

#define LABEL_TRAIT(n, t, u) template<> struct DataPointLabelTraits<DataPointLabel::n> { \
    using type = t; \
    static constexpr char const name[] = #n; \
    static constexpr char const unit[] = u; \
};

LABEL_TRAIT(ZEPC,                   std::string, "");
LABEL_TRAIT(State,                  std::string, "");
LABEL_TRAIT(BatteryGridState,       std::string, "");
LABEL_TRAIT(Temperature,            float,       "°C");
LABEL_TRAIT(Efficiency,             float,       "%");
LABEL_TRAIT(DayEnergy,              float,       "kWh");
LABEL_TRAIT(TotalEnergy,            float,       "kWh");
LABEL_TRAIT(AcVoltage,              float,       "V");
LABEL_TRAIT(MaxAcPower,             float,       "W");
LABEL_TRAIT(MinAcPower,             float,       "W");
LABEL_TRAIT(AcPowerSetpoint,        float,       "W");
LABEL_TRAIT(AcPower,                float,       "W");
LABEL_TRAIT(DcVoltage,              float,       "V");
LABEL_TRAIT(DcPower,                float,       "W");
LABEL_TRAIT(DcVoltageSetpoint,      float,       "V");
LABEL_TRAIT(DcCurrent,              float,       "A");
LABEL_TRAIT(DcVoltageOffline,       float,       "V");
LABEL_TRAIT(DcCurrentOffline,       float,       "A");
#undef LABEL_TRAIT

} // namespace GridChargers::Trucki

template class DataPointContainer<DataPoint<float, std::string>,
                                  GridChargers::Trucki::DataPointLabel,
                                  GridChargers::Trucki::DataPointLabelTraits>;

namespace GridChargers::Trucki {
    using DataPointContainer = ::DataPointContainer<::DataPoint<float, std::string>, DataPointLabel, DataPointLabelTraits>;
} // namespace GridChargers::Trucki
