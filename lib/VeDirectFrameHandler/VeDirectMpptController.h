#pragma once

#include <Arduino.h>
#include "VeDirectData.h"
#include "VeDirectFrameHandler.h"

template<typename T, size_t WINDOW_SIZE>
class MovingAverage {
public:
    MovingAverage()
      : _sum(0)
      , _index(0)
      , _count(0) { }

    void addNumber(T num) {
        if (_count < WINDOW_SIZE) {
            _count++;
        } else {
            _sum -= _window[_index];
        }

        _window[_index] = num;
        _sum += num;
        _index = (_index + 1) % WINDOW_SIZE;
    }

    float getAverage() const {
        if (_count == 0) { return 0.0; }
        return static_cast<float>(_sum) / _count;
    }

private:
    std::array<T, WINDOW_SIZE> _window;
    T _sum;
    size_t _index;
    size_t _count;
};

class VeDirectMpptController : public VeDirectFrameHandler<veMpptStruct> {
public:
    VeDirectMpptController() = default;

    void init(int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging, uint16_t hwSerialPort);

    using data_t = veMpptStruct;

    virtual void loop() final;                        // main loop to read ve.direct data

    struct veMPPTExStruct {
        int32_t T;                      // temperature [m°C] from internal MPPT sensor
        unsigned long Tts;              // time of last recieved value
        int32_t TSBS;                   // temperature [m°C] from the "Smart Battery Sense"
        unsigned long TSBSts;           // time of last recieved value
        uint32_t TDCP;                  // total DC input power [mW]
        unsigned long TDCPts;           // time of last recieved value
    };
    veMPPTExStruct _ExData{}; 
    veMPPTExStruct const *getExData() const { return &_ExData; }

private:
    void hexDataHandler(VeDirectHexData const &data) final;
    bool processTextDataDerived(std::string const& name, std::string const& value) final;
    void frameValidEvent() final;
    MovingAverage<float, 5> _efficiency;
    unsigned long _lastPingTime = 0L;               // time of last device PING/GET hex command
    bool _veMaster = true;                          // MPPT is instance master
};
