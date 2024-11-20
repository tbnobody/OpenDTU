#pragma once

#include <Arduino.h>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

using tCellVoltages = std::map<uint8_t, uint16_t>;

template<typename... V>
class DataPoint {
    template<typename, typename L, template<L> class>
    friend class DataPointContainer;

    public:
        using tValue = std::variant<V...>;

        DataPoint() = delete;

        DataPoint(DataPoint const& other)
            : _strLabel(other._strLabel)
            , _strValue(other._strValue)
            , _strUnit(other._strUnit)
            , _value(other._value)
            , _timestamp(other._timestamp) { }

        DataPoint(std::string const& strLabel, std::string const& strValue,
                std::string const& strUnit, tValue value, uint32_t timestamp)
            : _strLabel(strLabel)
            , _strValue(strValue)
            , _strUnit(strUnit)
            , _value(std::move(value))
            , _timestamp(timestamp) { }

        std::string const& getLabelText() const { return _strLabel; }
        std::string const& getValueText() const { return _strValue; }
        std::string const& getUnitText() const { return _strUnit; }
        uint32_t getTimestamp() const { return _timestamp; }

        bool operator==(DataPoint const& other) const {
            return _value == other._value;
        }

    private:
        std::string _strLabel;
        std::string _strValue;
        std::string _strUnit;
        tValue _value;
        uint32_t _timestamp;
};

template<typename T> std::string dataPointValueToStr(T const& v);

template<typename DataPoint, typename Label, template<Label> class Traits>
class DataPointContainer {
    public:
        DataPointContainer() = default;

        //template<Label L> using Traits = LabelTraits<L>;

        template<Label L>
        void add(typename Traits<L>::type val) {
            _dataPoints.emplace(
                    L,
                    DataPoint(
                        Traits<L>::name,
                        dataPointValueToStr(val),
                        Traits<L>::unit,
                        typename DataPoint::tValue(std::move(val)),
                        millis()
                    )
            );
        }

        // make sure add() is only called with the type expected for the
        // respective label, no implicit conversions allowed.
        template<Label L, typename T>
        void add(T) = delete;

        template<Label L>
        std::optional<DataPoint const> getDataPointFor() const {
            auto it = _dataPoints.find(L);
            if (it == _dataPoints.end()) { return std::nullopt; }
            return it->second;
        }

        template<Label L>
        std::optional<typename Traits<L>::type> get() const {
            auto optionalDataPoint = getDataPointFor<L>();
            if (!optionalDataPoint.has_value()) { return std::nullopt; }
            return std::get<typename Traits<L>::type>(optionalDataPoint->_value);
        }

        using tMap = std::unordered_map<Label, DataPoint const>;
        typename tMap::const_iterator cbegin() const { return _dataPoints.cbegin(); }
        typename tMap::const_iterator cend() const { return _dataPoints.cend(); }

        // copy all data points from source into this instance, overwriting
        // existing data points in this instance.
        void updateFrom(DataPointContainer const& source)
        {
            for (auto iter = source.cbegin(); iter != source.cend(); ++iter) {
                auto pos = _dataPoints.find(iter->first);

                if (pos != _dataPoints.end()) {
                    // do not update existing data points with the same value
                    if (pos->second == iter->second) { continue; }

                    _dataPoints.erase(pos);
                }
                _dataPoints.insert(*iter);
            }
        }

    private:
        tMap _dataPoints;
};
