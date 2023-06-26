/**
 *  @filename   :   epdif.h
 *  @brief      :   Header file of epdif.cpp providing EPD interface functions
 *                  Users have to implement all the functions in epdif.cpp
 *  @author     :   Yehui from Waveshare
 *
 *  Copyright (C) Waveshare     August 10 2017
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef EPDIF_H
#define EPDIF_H

#include <Arduino.h>

//************** SPI Belegung Fusion V2 *************************
// MISO(Busy)  14   // ePaper Busy indicator (SPI MISO aquivalent)
// RST         13   // ePaper Reset switch
// DC          12   // ePaper Data/Command selection
// CS(SS)      11   // SPI Channel Chip Selection for ePaper
// SCK(CLK)    10   // SPI Channel Click
// MOSI(DIN)   9   // SPI Channel MOSI Pin
//*************************************************************

class EpdIf {
public:
    EpdIf();
    ~EpdIf();

    uint8_t IfInit(void);
    void DigitalWrite(uint8_t pin, int value);
    int DigitalRead(uint8_t pin);
    void DelayMs(uint16_t delaytime);
    void SpiTransfer(uint8_t data);

    uint8_t EPD_SCK_PIN;
    uint8_t EPD_MOSI_PIN;
    uint8_t EPD_CS_PIN;
    uint8_t EPD_RST_PIN;
    uint8_t EPD_DC_PIN;
    uint8_t EPD_BUSY_PIN;

    uint8_t GPIO_PIN_SET = 1;
    uint8_t GPIO_PIN_RESET = 0;
};

#endif
