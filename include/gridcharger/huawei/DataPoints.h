#pragma once

#include <DataPoints.h>

namespace GridChargers::Huawei {

enum class DataPointLabel : uint8_t {
    // board properties message
    BoardType,
    Serial,
    Manufactured,
    VendorName,
    ProductName,
    ProductDescription,

    // device config message (except for max current multiplier)
    Reachable,
    Row,
    Slot,

    // acknowledgement messages
    OnlineVoltage,
    OfflineVoltage,
    OnlineCurrent,
    OfflineCurrent,
    ProductionEnabled,
    FanOnlineFullSpeed,
    FanOfflineFullSpeed,
    InputCurrentLimit,
    Mode,

    // rectifier state message
    InputPower = 0x70,
    InputFrequency = 0x71,
    InputCurrent = 0x72,
    OutputPower = 0x73,
    Efficiency = 0x74,
    OutputVoltage = 0x75,
    OutputCurrentMax = 0x76,
    InputVoltage = 0x78,
    OutputTemperature = 0x7F,
    InputTemperature = 0x80,
    OutputCurrent = 0x81
};

template<DataPointLabel> struct DataPointLabelTraits;

#define LABEL_TRAIT(n, t, u) template<> struct DataPointLabelTraits<DataPointLabel::n> { \
    using type = t; \
    static constexpr char const name[] = #n; \
    static constexpr char const unit[] = u; \
};

LABEL_TRAIT(BoardType,          std::string, "");
LABEL_TRAIT(Serial,             std::string, "");
LABEL_TRAIT(Manufactured,       std::string, "");
LABEL_TRAIT(VendorName,         std::string, "");
LABEL_TRAIT(ProductName,        std::string, "");
LABEL_TRAIT(ProductDescription, std::string, "");
LABEL_TRAIT(Reachable,          bool,        "");
LABEL_TRAIT(Row,                uint8_t,     "");
LABEL_TRAIT(Slot,               uint8_t,     "");
LABEL_TRAIT(OnlineVoltage,      float,       "V");
LABEL_TRAIT(OfflineVoltage,     float,       "V");
LABEL_TRAIT(OnlineCurrent,      float,       "A");
LABEL_TRAIT(OfflineCurrent,     float,       "A");
LABEL_TRAIT(ProductionEnabled,  bool,        "");
LABEL_TRAIT(FanOnlineFullSpeed, bool,        "");
LABEL_TRAIT(FanOfflineFullSpeed,bool,        "");
LABEL_TRAIT(InputCurrentLimit,  float,       "A");
LABEL_TRAIT(Mode,               uint8_t,     "");
LABEL_TRAIT(InputPower,         float,       "W");
LABEL_TRAIT(InputFrequency,     float,       "Hz");
LABEL_TRAIT(InputCurrent,       float,       "A");
LABEL_TRAIT(OutputPower,        float,       "W");
LABEL_TRAIT(Efficiency,         float,       "%");
LABEL_TRAIT(OutputVoltage,      float,       "V");
LABEL_TRAIT(OutputCurrentMax,   float,       "A");
LABEL_TRAIT(InputVoltage,       float,       "V");
LABEL_TRAIT(OutputTemperature,  float,       "°C");
LABEL_TRAIT(InputTemperature,   float,       "°C");
LABEL_TRAIT(OutputCurrent,      float,       "A");
#undef LABEL_TRAIT

} // namespace GridChargers::Huawei

template class DataPointContainer<DataPoint<float, std::string, uint8_t, bool>,
                                  GridChargers::Huawei::DataPointLabel,
                                  GridChargers::Huawei::DataPointLabelTraits>;

namespace GridChargers::Huawei {
    using DataPointContainer = DataPointContainer<DataPoint<float, std::string, uint8_t, bool>, DataPointLabel, DataPointLabelTraits>;
} // namespace GridChargers::Huawei
