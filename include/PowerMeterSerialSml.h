// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "PowerMeterSml.h"
#include <SoftwareSerial.h>

class PowerMeterSerialSml : public PowerMeterSml {
public:
    PowerMeterSerialSml()
        : PowerMeterSml("PowerMeterSerialSml") { }

    ~PowerMeterSerialSml();

    bool init() final;
    void loop() final;

private:
    // we assume that an SML datagram is complete after no additional
    // characters were received for this many milliseconds.
    static uint8_t constexpr _datagramGapMillis = 50;

    static uint32_t constexpr _baud = 9600;

    // size in bytes of the software serial receive buffer. must have the
    // capacity to hold a full SML datagram, as we are processing the datagrams
    // only after all data of one datagram was received.
    static int constexpr _bufCapacity = 1024; // memory usage: 1 byte each

    // amount of bits (RX pin state transitions) the software serial can buffer
    // without decoding bits to bytes and storing those in the receive buffer.
    // this value dictates how ofter we need to call a function of the software
    // serial instance that performs bit decoding (we call available()).
    static int constexpr _isrCapacity = 256; // memory usage: 8 bytes each (timestamp + pointer)

    static void pollingLoopHelper(void* context);
    std::atomic<bool> _taskDone;
    void pollingLoop();

    TaskHandle_t _taskHandle = nullptr;
    bool _stopPolling;
    mutable std::mutex _pollingMutex;

    std::unique_ptr<SoftwareSerial> _upSmlSerial = nullptr;
};
