/*****************************************************************************
* | File      	:   epd1in54_V2.h
* | Author      :   Waveshare team
* | Function    :   1.54inch e-paper V2
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2019-06-24
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/

#ifndef epd1in54_V2_H
#define epd1in54_V2_H

#include "epdif.h"

// Display resolution
#define EPD_WIDTH 200
#define EPD_HEIGHT 200

class Epd : EpdIf {
public:
    unsigned long width;
    unsigned long height;

    Epd(uint8_t _CS, uint8_t _DC, uint8_t _RST, uint8_t _BUSY, uint8_t _SCK, uint8_t _MOSI);
    ~Epd();
    // int  Init(void);
    int LDirInit(void);
    int HDirInit(void);
    void SendCommand(unsigned char command);
    void SendData(unsigned char data);
    void WaitUntilIdle(void);
    void Reset(void);
    void Clear(void);
    void Display(const unsigned char* frame_buffer);
    void DisplayPartBaseImage(const unsigned char* frame_buffer);
    void DisplayPartBaseWhiteImage(void);
    void DisplayPart(const unsigned char* frame_buffer);
    void SetFrameMemory(
        const unsigned char* image_buffer,
        int x,
        int y,
        int image_width,
        int image_height);
    void SetFrameMemoryPartial(
        const unsigned char* image_buffer,
        int x,
        int y,
        int image_width,
        int image_height);
    void DisplayFrame(void);
    void DisplayPartFrame(void);

    void Sleep(void);

private:
    void Lut(unsigned char* lut);
    void SetLut(unsigned char* lut);
    void SetMemoryArea(int x_start, int y_start, int x_end, int y_end);
    void SetMemoryPointer(int x, int y);
};

#endif /* EPD1IN54B_H */

/* END OF FILE */
