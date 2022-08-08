#pragma once
#include <cstdint>

class Parser {
public:
    uint32_t getLastUpdate();
    void setLastUpdate(uint32_t lastUpdate);

private:
    uint32_t _lastUpdate = 0;
};