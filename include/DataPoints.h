#pragma once

#include <Arduino.h>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <limits>
#include <algorithm>
#include <mutex>

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

        DataPointContainer(DataPointContainer const& other)
        {
            auto scopedLock = other.lock();
            _dataPoints = other._dataPoints;
        }

        // allows to keep the container locked while adding multiple data points
        // that are supposed to be coherent, and/or to ensure thread safety.
        std::unique_lock<std::mutex> lock() const {
            return std::unique_lock<std::mutex>(_mutex);
        }

        // add a data point to the container. if the data point already exists,
        // its value will be replaced. if ignoreAge is true, the timestamp will not
        // be set, which means that this data point is "timeless" and hence is
        // ignored in the getLastUpdate() method.
        template<Label L>
        void add(typename Traits<L>::type val, bool ignoreAge = false) {
            // no locking here! iff thread safety is required, use the lock()
            // method in a scoped block in which this method is called, as we
            // expect that usually multiple data points are added at a time.

            _dataPoints.erase(L);

            uint32_t timestamp = ignoreAge ? 0 : millis();
            if (!ignoreAge && timestamp == 0) { timestamp = 1; }

            _dataPoints.emplace(
                    L,
                    DataPoint(
                        Traits<L>::name,
                        dataPointValueToStr(val),
                        Traits<L>::unit,
                        typename DataPoint::tValue(std::move(val)),
                        timestamp
                    )
            );
        }

        // make sure add() is only called with the type expected for the
        // respective label, no implicit conversions allowed.
        template<Label L, typename T>
        void add(T) = delete;

        template<Label L>
        std::optional<DataPoint const> getDataPointFor() const {
            auto scopedLock = lock();

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
            auto scopedLock = lock();
            auto otherScopedLock = source.lock();

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

        uint32_t getLastUpdate() const
        {
            auto scopedLock = lock();

            if (_dataPoints.empty()) { return 0; }

            uint32_t now = millis();
            std::optional<uint32_t> oDiff;
            for (auto iter = _dataPoints.cbegin(); iter != _dataPoints.cend(); ++iter) {
                if (iter->second.getTimestamp() == 0) { continue; } // "timeless" data points are ignored
                oDiff = std::min(oDiff.value_or(std::numeric_limits<uint32_t>::max()/2), now - iter->second.getTimestamp());
            }

            // no data point with a timestamp yet
            if (!oDiff) { return 0; }

            return now - *oDiff;
        }

        void clear() {
            auto scopedLock = lock();
            _dataPoints.clear();
        }

    private:
        tMap _dataPoints;
        mutable std::mutex _mutex;
};
