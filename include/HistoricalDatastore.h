// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>
#include <Hoymiles.h>
#include <TaskSchedulerDeclarations.h>
#include <cstdint>
#include <map>
#include <mutex>
#include <vector>
#include <time.h>

// Max number of data points to store for each time resolution
#define MAX_DATA_POINTS_20MIN 72     // 24 hours (72 * 20 minutes)
#define MAX_DATA_POINTS_HOURLY 168   // 7 days (24 * 7 hours)
#define MAX_DATA_POINTS_DAILY 60     // 2 months (60 days)
#define MAX_DATA_POINTS_MONTHLY 24   // 2 years (24 months)

// Data collection interval in milliseconds
#define COLLECT_INTERVAL_20MIN 20*60*1000    // 20 minutes
#define COLLECT_INTERVAL_HOURLY 60*60*1000   // 1 hour

struct PowerDataPoint {
    time_t timestamp;         // Unix timestamp
    float yieldValue;         // Power production value in Wh
    float peakPowerValue;     // Peak power value in W during the period
};

typedef std::vector<PowerDataPoint> DataPointCollection;

class HistoricalDatastoreClass {
public:
    HistoricalDatastoreClass();
    void init(Scheduler& scheduler);

    // Get data points for various time periods
    const DataPointCollection& get20MinData() const;
    const DataPointCollection& getHourlyData() const;
    const DataPointCollection& getDailyData() const;
    const DataPointCollection& getMonthlyData() const;

    // Get data for a specific time range
    DataPointCollection getDataRange(time_t startTime, time_t endTime, uint8_t resolution = 0);

    // Manual data reset
    void resetData();

private:
    void collect20MinData();
    void collectHourlyData();
    void aggregateDailyData();
    void aggregateMonthlyData();
    
    void checkAndInitializeNewDay();
    void checkAndInitializeNewMonth();
    
    // Tasks for data collection
    Task _collect20MinTask;
    Task _collectHourlyTask;
    Task _aggregateDailyTask;
    Task _aggregateMonthlyTask;
    
    // Data storage
    DataPointCollection _20minData;
    DataPointCollection _hourlyData;
    DataPointCollection _dailyData;
    DataPointCollection _monthlyData;
    
    // Keep track of the last captured day and month
    int _lastDay;
    int _lastMonth;
    int _lastYear;
    
    // Peak power tracking for current periods
    float _currentHourPeakPower;
    float _currentDayPeakPower;
    float _currentMonthPeakPower;
    
    // Current day and month yield accumulation
    float _currentDayYield;
    float _currentMonthYield;
    
    // Thread safety
    std::mutex _mutex;
};

extern HistoricalDatastoreClass HistoricalDatastore;
