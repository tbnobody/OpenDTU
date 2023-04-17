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

    bool startListening(void);

    bool stopListening(void);

    /**
     * Check whether there are bytes available to be read
     * @code
     * if(radio.available()){
     *   radio.read(&data,sizeof(data));
     * }
     * @endcode
     *
     * @see available(uint8_t*)
     *
     * @return True if there is a payload available, false if none is
     */
    bool available(void);

    /**
     * Read payload data from the RX FIFO buffer(s).
     *
     * The length of data read is usually the next available payload's length
     * @see
     * - getDynamicPayloadSize()
     *
     * @note I specifically chose `void*` as a data type to make it easier
     * for beginners to use.  No casting needed.
     *
     * @param buf Pointer to a buffer where the data should be written
     * @param len Maximum number of bytes to read into the buffer. This
     * value should match the length of the object referenced using the
     * `buf` parameter. The absolute maximum number of bytes that can be read
     * in one call is 32 (for dynamic payload lengths) or whatever number was
     * previously passed to setPayloadSize() (for static payload lengths).
     */
    void read(void* buf, uint8_t len);

    bool write(const uint8_t* buf, uint8_t len);

    /**
     * Set RF communication channel. The frequency used by a channel is
     * @param channel Which RF channel to communicate on, 0-254
     */
    void setChannel(uint8_t channel);

    /**
     * Get RF communication channel
     * @return The currently configured RF Channel
     */
    uint8_t getChannel(void);

    /**
     * Get Dynamic Payload Size
     *
     * For dynamic payloads, this pulls the size of the payload off
     * the chip
     *
     * @return Payload length of last-received dynamic payload
     */
    uint8_t getDynamicPayloadSize(void);

    int getRssiDBm();

    bool setPALevel(int8_t level);

    bool rxFifoAvailable();

    /**
     * Empty the RX (receive) FIFO buffers.
     */
    void flush_rx(void);

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