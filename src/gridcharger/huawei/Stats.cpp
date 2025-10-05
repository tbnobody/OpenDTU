// SPDX-License-Identifier: GPL-2.0-or-later
#include <Configuration.h>
#include <MqttSettings.h>
#include <MessageOutput.h>
#include <gridcharger/huawei/Stats.h>

namespace GridChargers::Huawei {

uint32_t Stats::getLastUpdate() const
{
    return _dataPoints.getLastUpdate();
}

std::optional<float> Stats::getInputPower() const
{
   return _dataPoints.get<DataPointLabel::InputPower>();
}

void Stats::mqttPublish() const
{
    #define PUB(l, t) \
    { \
        auto oDataPoint = _dataPoints.get<GridChargers::Huawei::DataPointLabel::l>(); \
        if (oDataPoint) { \
            MqttSettings.publish("huawei/" t, String(*oDataPoint)); \
        } \
    }

    PUB(InputVoltage, "input_voltage");
    PUB(InputCurrent, "input_current");
    PUB(InputPower, "input_power");
    PUB(InputFrequency, "input_frequency");
    PUB(OutputVoltage, "output_voltage");
    PUB(OutputCurrent, "output_current");
    PUB(OutputCurrentMax, "max_output_current");
    PUB(OutputPower, "output_power");
    PUB(InputTemperature, "input_temp");
    PUB(OutputTemperature, "output_temp");
    PUB(Efficiency, "efficiency");
    PUB(Row, "slot_detection/row");
    PUB(Slot, "slot_detection/slot");
    PUB(Mode, "mode");
#undef PUB

#define PUBACK(l, t) \
    { \
        auto oDataPoint = _dataPoints.get<GridChargers::Huawei::DataPointLabel::l>(); \
        if (oDataPoint) { \
            MqttSettings.publish("huawei/acks/" t, String(*oDataPoint)); \
        } \
    }

    PUBACK(OnlineVoltage, "online_voltage");
    PUBACK(OfflineVoltage, "offline_voltage");
    PUBACK(OnlineCurrent, "online_current");
    PUBACK(OfflineCurrent, "offline_current");
    PUBACK(ProductionEnabled, "production_enabled");
    PUBACK(FanOnlineFullSpeed, "fan_online_full_speed");
    PUBACK(FanOfflineFullSpeed, "fan_offline_full_speed");
    PUBACK(InputCurrentLimit, "input_current_limit");
#undef PUBACK


#define PUBSTR(l, t) \
    { \
        auto oDataPoint = _dataPoints.get<GridChargers::Huawei::DataPointLabel::l>(); \
        if (oDataPoint) { \
            MqttSettings.publish("huawei/" t, String(oDataPoint->c_str())); \
        } \
    }

    PUBSTR(BoardType, "board_type");
    PUBSTR(Serial, "serial");
    PUBSTR(Manufactured, "manufactured");
    PUBSTR(VendorName, "vendor_name");
    PUBSTR(ProductName, "product_name");
    PUBSTR(ProductDescription, "product_description");
#undef PUBSTR

    auto const& oReachable = _dataPoints.get<GridChargers::Huawei::DataPointLabel::Reachable>();
    if (oReachable) {
        MqttSettings.publish("huawei/reachable", String(*oReachable?1:0));
    }

    MqttSettings.publish("huawei/data_age", String((millis() - _dataPoints.getLastUpdate()) / 1000));
}

void Stats::updateFrom(DataPointContainer const& dataPoints) const
{
    _dataPoints.updateFrom(dataPoints);
}

void Stats::getLiveViewData(JsonVariant& root) const
{
    root["dataAge"] = millis() - _dataPoints.getLastUpdate();
    root["provider"] = GridChargerProviderType::HUAWEI;

    auto oReachable = _dataPoints.get<DataPointLabel::Reachable>();
    root["reachable"] = oReachable.value_or(false);

    auto oOutputPower = _dataPoints.get<DataPointLabel::OutputPower>();
    auto oOutputCurrent = _dataPoints.get<DataPointLabel::OutputCurrent>();
    root["producing"] = oOutputPower.value_or(0) > 10 && oOutputCurrent.value_or(0) > 0.1;

#define VAL(l, n) \
    { \
        auto oVal = _dataPoints.get<DataPointLabel::l>(); \
        if (oVal) { root[n] = *oVal; } \
    }

    VAL(Serial,              "serial");
    VAL(VendorName,          "vendorName");
    VAL(ProductName,         "productName");
#undef VAL

    addStringInSection<DataPointLabel::BoardType>(root, "device", "boardType", false);
    addStringInSection<DataPointLabel::Manufactured>(root, "device", "manufactured", false);
    addStringInSection<DataPointLabel::ProductDescription>(root, "device", "productDescription", false);
    addStringInSection<DataPointLabel::Row>(root, "device", "row", false);
    addStringInSection<DataPointLabel::Slot>(root, "device", "slot", false);

    addValueInSection<DataPointLabel::InputVoltage>(root, "input", "voltage");
    addValueInSection<DataPointLabel::InputCurrent>(root, "input", "current");
    addValueInSection<DataPointLabel::InputPower>(root, "input", "power");
    addValueInSection<DataPointLabel::InputTemperature>(root, "input", "temp");
    addValueInSection<DataPointLabel::InputFrequency>(root, "input", "frequency");
    addValueInSection<DataPointLabel::Efficiency>(root, "input", "efficiency");

    addValueInSection<DataPointLabel::OutputVoltage>(root, "output", "voltage");
    addValueInSection<DataPointLabel::OutputCurrent>(root, "output", "current");
    addValueInSection<DataPointLabel::OutputPower>(root, "output", "power");
    addValueInSection<DataPointLabel::OutputTemperature>(root, "output", "temp");
    addValueInSection<DataPointLabel::OutputCurrentMax>(root, "output", "maxCurrent");

    addValueInSection<DataPointLabel::OnlineVoltage>(root, "acknowledgements", "onlineVoltage");
    addValueInSection<DataPointLabel::OfflineVoltage>(root, "acknowledgements", "offlineVoltage");
    addValueInSection<DataPointLabel::OnlineCurrent>(root, "acknowledgements", "onlineCurrent");
    addValueInSection<DataPointLabel::OfflineCurrent>(root, "acknowledgements", "offlineCurrent");
    addValueInSection<DataPointLabel::InputCurrentLimit>(root, "acknowledgements", "inputCurrentLimit");

    auto oProductionEnabled = _dataPoints.get<DataPointLabel::ProductionEnabled>();
    if (oProductionEnabled) {
        ::GridChargers::Stats::addStringInSection(root, "acknowledgements", "productionEnabled", *oProductionEnabled?"yes":"no");
    }

    auto oFanOnlineFullSpeed = _dataPoints.get<DataPointLabel::FanOnlineFullSpeed>();
    if (oFanOnlineFullSpeed) {
        ::GridChargers::Stats::addStringInSection(root, "acknowledgements", "fanOnlineFullSpeed", *oFanOnlineFullSpeed?"FanFullSpeed":"FanAuto");
    }

    auto oFanOfflineFullSpeed = _dataPoints.get<DataPointLabel::FanOfflineFullSpeed>();
    if (oFanOfflineFullSpeed) {
        ::GridChargers::Stats::addStringInSection(root, "acknowledgements", "fanOfflineFullSpeed", *oFanOfflineFullSpeed?"FanFullSpeed":"FanAuto");
    }
}

}; // namespace GridChargers::Huawei
