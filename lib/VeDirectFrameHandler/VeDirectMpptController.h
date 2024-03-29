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

class VeDirectMpptController : public VeDirectFrameHandler<veMpptStruct> {
public:
    VeDirectMpptController() = default;

    void init(int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging, uint16_t hwSerialPort);

    using data_t = veMpptStruct;
    using spData_t = std::shared_ptr<data_t const>;
    spData_t getData() const { return _spData; }

private:
    bool processTextDataDerived(std::string const& name, std::string const& value) final;
    void frameValidEvent() final;
    spData_t _spData = nullptr;
    MovingAverage<double, 5> _efficiency;
};
