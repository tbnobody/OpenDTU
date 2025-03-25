// SPDX-License-Identifier: GPL-2.0-or-later

#include "HistoricalDatastore.h"
#include "Datastore.h"
#include <time.h>
#include <algorithm>

HistoricalDatastoreClass HistoricalDatastore;

HistoricalDatastoreClass::HistoricalDatastoreClass()
    : _collect20MinTask(COLLECT_INTERVAL_20MIN, TASK_FOREVER, std::bind(&HistoricalDatastoreClass::collect20MinData, this))
    , _collectHourlyTask(COLLECT_INTERVAL_HOURLY, TASK_FOREVER, std::bind(&HistoricalDatastoreClass::collectHourlyData, this))
    , _aggregateDailyTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&HistoricalDatastoreClass::aggregateDailyData, this))
    , _aggregateMonthlyTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&HistoricalDatastoreClass::aggregateMonthlyData, this))
    , _lastDay(-1)
    , _lastMonth(-1)
    , _lastYear(-1)
    , _currentHourPeakPower(0)
    , _currentDayPeakPower(0)
    , _currentMonthPeakPower(0)
    , _currentDayYield(0)
    , _currentMonthYield(0)
{
    // Reserve space for data collections to avoid reallocations
    _20minData.reserve(MAX_DATA_POINTS_20MIN);
    _hourlyData.reserve(MAX_DATA_POINTS_HOURLY);
    _dailyData.reserve(MAX_DATA_POINTS_DAILY);
    _monthlyData.reserve(MAX_DATA_POINTS_MONTHLY);
}

void HistoricalDatastoreClass::init(Scheduler& scheduler)
{
    // Add and enable tasks
    scheduler.addTask(_collect20MinTask);
    scheduler.addTask(_collectHourlyTask);
    scheduler.addTask(_aggregateDailyTask);
    scheduler.addTask(_aggregateMonthlyTask);
    
    _collect20MinTask.enable();
    _collectHourlyTask.enable();
    
    // Daily & monthly tasks run at midnight checks
    _aggregateDailyTask.setInterval(60 * 1000); // Check every minute
    _aggregateMonthlyTask.setInterval(60 * 1000); // Check every minute
    
    _aggregateDailyTask.enable();
    _aggregateMonthlyTask.enable();
}

void HistoricalDatastoreClass::collect20MinData()
{
    std::lock_guard<std::mutex> lock(_mutex);
    
    time_t now;
    time(&now);
    
    // Get current power production
    float currentPower = Datastore.getTotalAcPowerEnabled();
    
    // Calculate yield over 20 minutes (power in W * time in h = energy in Wh)
    float yieldValue = (currentPower * (20.0 / 60.0));
    
    // Track peak power for higher-level aggregations
    _currentHourPeakPower = std::max(_currentHourPeakPower, currentPower);
    _currentDayPeakPower = std::max(_currentDayPeakPower, currentPower);
    _currentMonthPeakPower = std::max(_currentMonthPeakPower, currentPower);
    
    // Accumulate for daily and monthly totals
    _currentDayYield += yieldValue;
    _currentMonthYield += yieldValue;
    
    // Add data point
    PowerDataPoint point = {
        .timestamp = now,
        .yieldValue = yieldValue,
        .peakPowerValue = currentPower
    };
    
    // Add to 20-minute collection and maintain maximum size
    _20minData.push_back(point);
    if (_20minData.size() > MAX_DATA_POINTS_20MIN) {
        _20minData.erase(_20minData.begin());
    }
}

void HistoricalDatastoreClass::collectHourlyData()
{
    std::lock_guard<std::mutex> lock(_mutex);
    
    time_t now;
    time(&now);
    
    // Get daily yield - this should be the sum of the last 3 20-minute values
    float hourlyYield = 0;
    if (_20minData.size() >= 3) {
        for (size_t i = 1; i <= 3 && i <= _20minData.size(); i++) {
            hourlyYield += _20minData[_20minData.size() - i].yieldValue;
        }
    }
    
    // Add data point
    PowerDataPoint point = {
        .timestamp = now,
        .yieldValue = hourlyYield,
        .peakPowerValue = _currentHourPeakPower
    };
    
    // Add to hourly collection and maintain maximum size
    _hourlyData.push_back(point);
    if (_hourlyData.size() > MAX_DATA_POINTS_HOURLY) {
        _hourlyData.erase(_hourlyData.begin());
    }
    
    // Reset hour peak
    _currentHourPeakPower = 0;
}

void HistoricalDatastoreClass::aggregateDailyData()
{
    std::lock_guard<std::mutex> lock(_mutex);
    
    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Check if it's a new day
    if (timeinfo.tm_mday != _lastDay || timeinfo.tm_mon + 1 != _lastMonth || timeinfo.tm_year + 1900 != _lastYear) {
        // Only record data if this isn't the first run
        if (_lastDay != -1) {
            // Create timestamp for start of previous day
            struct tm yesterday = timeinfo;
            yesterday.tm_mday = _lastDay;
            yesterday.tm_mon = _lastMonth - 1; // tm_mon is 0-based
            yesterday.tm_year = _lastYear - 1900; // tm_year is years since 1900
            yesterday.tm_hour = 0;
            yesterday.tm_min = 0;
            yesterday.tm_sec = 0;
            time_t yesterdayTime = mktime(&yesterday);
            
            // Add data point for the completed day
            PowerDataPoint point = {
                .timestamp = yesterdayTime,
                .yieldValue = _currentDayYield,
                .peakPowerValue = _currentDayPeakPower
            };
            
            // Add to daily collection and maintain maximum size
            _dailyData.push_back(point);
            if (_dailyData.size() > MAX_DATA_POINTS_DAILY) {
                _dailyData.erase(_dailyData.begin());
            }
        }
        
        // Update day tracking and reset accumulators
        _lastDay = timeinfo.tm_mday;
        _lastMonth = timeinfo.tm_mon + 1;
        _lastYear = timeinfo.tm_year + 1900;
        _currentDayYield = 0;
        _currentDayPeakPower = 0;
    }
}

void HistoricalDatastoreClass::aggregateMonthlyData()
{
    std::lock_guard<std::mutex> lock(_mutex);
    
    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Check if it's a new month
    if ((timeinfo.tm_mon + 1 != _lastMonth || timeinfo.tm_year + 1900 != _lastYear) && _lastMonth != -1) {
        // Create timestamp for start of previous month
        struct tm lastMonth = timeinfo;
        lastMonth.tm_mday = 1;
        lastMonth.tm_mon = _lastMonth - 1; // tm_mon is 0-based
        lastMonth.tm_year = _lastYear - 1900; // tm_year is years since 1900
        lastMonth.tm_hour = 0;
        lastMonth.tm_min = 0;
        lastMonth.tm_sec = 0;
        time_t lastMonthTime = mktime(&lastMonth);
        
        // Add data point for the completed month
        PowerDataPoint point = {
            .timestamp = lastMonthTime,
            .yieldValue = _currentMonthYield,
            .peakPowerValue = _currentMonthPeakPower
        };
        
        // Add to monthly collection and maintain maximum size
        _monthlyData.push_back(point);
        if (_monthlyData.size() > MAX_DATA_POINTS_MONTHLY) {
            _monthlyData.erase(_monthlyData.begin());
        }
        
        // Reset month accumulators
        _currentMonthYield = 0;
        _currentMonthPeakPower = 0;
    }
}

const DataPointCollection& HistoricalDatastoreClass::get20MinData() const
{
    return _20minData;
}

const DataPointCollection& HistoricalDatastoreClass::getHourlyData() const
{
    return _hourlyData;
}

const DataPointCollection& HistoricalDatastoreClass::getDailyData() const
{
    return _dailyData;
}

const DataPointCollection& HistoricalDatastoreClass::getMonthlyData() const
{
    return _monthlyData;
}

DataPointCollection HistoricalDatastoreClass::getDataRange(time_t startTime, time_t endTime, uint8_t resolution)
{
    std::lock_guard<std::mutex> lock(_mutex);
    
    const DataPointCollection* sourceData;
    
    // Select the appropriate data source based on resolution
    switch(resolution) {
        case 0: // 20 minute data
            sourceData = &_20minData;
            break;
        case 1: // Hourly data
            sourceData = &_hourlyData;
            break;
        case 2: // Daily data
            sourceData = &_dailyData;
            break;
        case 3: // Monthly data
            sourceData = &_monthlyData;
            break;
        default:
            sourceData = &_hourlyData; // Default to hourly
    }
    
    // Filter data points within the requested time range
    DataPointCollection result;
    for (const auto& point : *sourceData) {
        if (point.timestamp >= startTime && point.timestamp <= endTime) {
            result.push_back(point);
        }
    }
    
    return result;
}

void HistoricalDatastoreClass::resetData()
{
    std::lock_guard<std::mutex> lock(_mutex);
    
    // Clear all data collections
    _20minData.clear();
    _hourlyData.clear();
    _dailyData.clear();
    _monthlyData.clear();
    
    // Reset accumulators
    _currentHourPeakPower = 0;
    _currentDayPeakPower = 0;
    _currentMonthPeakPower = 0;
    _currentDayYield = 0;
    _currentMonthYield = 0;
}
