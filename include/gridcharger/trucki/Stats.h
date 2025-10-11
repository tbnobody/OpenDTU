// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <gridcharger/Stats.h>
#include <gridcharger/trucki/DataPoints.h>

namespace GridChargers::Trucki {

class Stats : public ::GridChargers::Stats {
friend class Provider;

public:
    uint32_t getLastUpdate() const final;

    std::optional<float> getInputPower() const final;

    void getLiveViewData(JsonVariant& root) const final;

protected:
    void mqttPublish() const final {}

    void updateFrom(DataPointContainer const& dataPoints) const;

private:
    template<DataPointLabel L>
    void addValueInSection(JsonVariant& root,
        std::string const& section, std::string const& name,
        int precision = 2) const
    {
        auto oVal = _dataPoints.get<L>();
        if (!oVal) { return; }

        ::GridChargers::Stats::addValueInSection(root, section, name, *oVal, DataPointLabelTraits<L>::unit, precision);
    }

    template<DataPointLabel L>
    void addStringInSection(JsonVariant& root,
        std::string const& section, std::string const& name,
        bool translate = true) const
    {
        auto oVal = _dataPoints.get<L>();
        if (!oVal) { return; }

        ::GridChargers::Stats::addStringInSection(root, section, name, *oVal, translate);
    }

    mutable DataPointContainer _dataPoints;
};

} // namespace GridChargers::Trucki
