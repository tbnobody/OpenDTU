#pragma once

#include <Arduino.h>
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

    double getAverage() const {
        if (_count == 0) { return 0.0; }
        return static_cast<double>(_sum) / _count;
    }

private:
    std::array<T, WINDOW_SIZE> _window;
    T _sum;
    size_t _index;
    size_t _count;
};

class VeDirectMpptController : public VeDirectFrameHandler {
public:
    VeDirectMpptController();

    void init(int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging);
    String getMpptAsString(uint8_t mppt);    // state of mppt as string
    String getCsAsString(uint8_t cs);        // current state as string
    String getOrAsString(uint32_t offReason); // off reason as string
    bool isDataValid();                      // return true if data valid and not outdated

    struct veMpptStruct : veStruct {
        uint8_t  MPPT;                  // state of MPP tracker
        int32_t PPV;                    // panel power in W
        double VPV;                     // panel voltage in V
        double IPV;                     // panel current in A (calculated)
        bool LOAD;                      // virtual load output state (on if battery voltage reaches upper limit, off if battery reaches lower limit)
        uint8_t  CS;                    // current state of operation e. g. OFF or Bulk
        uint8_t ERR;                    // error code
        uint32_t OR;                    // off reason
        uint32_t HSDS;                  // day sequence number 1...365
        double H19;                     // yield total kWh
        double H20;                     // yield today kWh
        int32_t H21;                    // maximum power today W
        double H22;                     // yield yesterday kWh
        int32_t H23;                    // maximum power yesterday W
    };

    veMpptStruct veFrame{};

private:
    void textRxEvent(char * name, char * value) final;
    void frameEndEvent(bool) final;                  // copy temp struct to public struct
    veMpptStruct _tmpFrame{};                        // private struct for received name and value pairs
    MovingAverage<double, 5> _efficiency;
};

extern VeDirectMpptController VeDirectMppt;