// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <Arduino.h>
#include <cstdint>

#define HOY_SEMAPHORE_TAKE() \
    do {                     \
    } while (xSemaphoreTake(_xSemaphore, portMAX_DELAY) != pdPASS)
#define HOY_SEMAPHORE_GIVE() xSemaphoreGive(_xSemaphore)

typedef enum {
    CMD_OK,
    CMD_NOK,
    CMD_PENDING
} LastCommandSuccess;

class Parser {
public:
    Parser();
    uint32_t getLastUpdate() const;
    void setLastUpdate(const uint32_t lastUpdate);

    void beginAppendFragment();
    void endAppendFragment();

protected:
    SemaphoreHandle_t _xSemaphore;

private:
    uint32_t _lastUpdate = 0;
};