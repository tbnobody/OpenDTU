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
    VeDirectMpptController() = default;

    void init(int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging);
    bool isDataValid() const;                        // return true if data valid and not outdated

    struct veMpptStruct : veStruct {
        uint8_t  MPPT;                  // state of MPP tracker
        int32_t PPV;                    // panel power in W
        int32_t P;                      // battery output power in W (calculated)
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

        String getMpptAsString() const; // state of mppt as string
        String getCsAsString() const;   // current state as string
        String getErrAsString() const;  // error state as string
        String getOrAsString() const;   // off reason as string
    };

    using spData_t = std::shared_ptr<veMpptStruct const>;
    spData_t getData() const { return _spData; }

private:
    void textRxEvent(char* name, char* value) final;
    void frameValidEvent() final;
    spData_t _spData = nullptr;
    veMpptStruct _tmpFrame{};                        // private struct for received name and value pairs
    MovingAverage<double, 5> _efficiency;
};
