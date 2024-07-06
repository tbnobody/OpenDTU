#pragma once

#include <TaskSchedulerDeclarations.h>
#include "FS.h"

class DataLoggerClass {
public:
    DataLoggerClass();
    void init(Scheduler& scheduler);

    void logToSDCard();
    void setSaveInterval(const uint32_t interval);
private:
    void loop();

    Task _loopTask;

    uint32_t _saveInterval = 0;
    uint32_t _lastSave = 0;

    void writeFile(FS &fs, const String path, const char * message);
    void appendFile(FS &fs, const String path, const char * message);

    uint64_t getTime();
};

extern DataLoggerClass DataLogger;
