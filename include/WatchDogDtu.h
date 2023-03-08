#pragma once

#include <cstdint>
#include "Configuration.h"
#include <esp_task_wdt.h>

class WatchDogDtuClass {
public:
    void init();
    void loop();

private:
    uint32_t _lastPublish;
};

extern WatchDogDtuClass WatchDogDtu;