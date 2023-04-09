// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdint.h>

#define CMT2300A_ONE_STEP_SIZE 2500 // frequency channel step size for fast frequency hopping operation: One step size is 2.5 kHz.
#define CMT_BASE_FREQ 860000000 // from Frequency Bank in cmt2300a_params.h
#define FH_OFFSET 100 // value * CMT2300A_ONE_STEP_SIZE = channel frequency offset
#define CMT_SPI_SPEED 4000000 // 4 MHz

class CMT2300A {
public:
    CMT2300A(uint8_t pin_sdio, uint8_t pin_clk, uint8_t pin_cs, uint8_t pin_fcs, uint32_t _spi_speed = CMT_SPI_SPEED);

    bool begin(void);

    /**
     * Checks if the chip is connected to the SPI bus
     */
    bool isChipConnected();

    bool write(const uint8_t* buf, uint8_t len);

    bool setPALevel(int8_t level);

private:
    /**
     * initialize the GPIO pins
     */
    bool _init_pins();

    /**
     * initialize radio.
     * @warning This function assumes the SPI bus object's begin() method has been
     * previously called.
     */
    bool _init_radio();

    int8_t _pin_sdio;
    int8_t _pin_clk;
    int8_t _pin_cs;
    int8_t _pin_fcs;
    uint32_t _spi_speed;
};