/**
 *  @filename   :   epdpaint.h
 *  @brief      :   Header file for epdpaint.cpp
 *  @author     :   Yehui from Waveshare
 *
 *  Copyright (C) Waveshare     July 28 2017
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

#ifndef EPDPAINT_H
#define EPDPAINT_H

// Display orientation
#define ROTATE_0 0
#define ROTATE_90 1
#define ROTATE_180 2
#define ROTATE_270 3

// Color inverse. 1 or 0 = set or reset a bit if set a colored pixel
#define IF_INVERT_COLOR 1

#include "fonts.h"
// #include <Adafruit_GFX.h>

/*
class Paint : public Adafruit_GFX {
    public:
Paint(unsigned char* image, int16_t width, int16_t height)
       : Adafruit_GFX(width, height)
        , _image(image)
        , _rotate(ROTATE_0)
        , _width((width + 7) / 8 * 8)
        , _height((height + 7) / 8 * 8)
    {
    }

     void drawPixel(int16_t x, int16_t y, uint16_t color)
     {
         Adafruit_GFX::drawPixel(x, y, color);
     }

    void Paint_DrawImage(const unsigned char* image_buffer, int16_t xStart, int16_t yStart, int16_t W_Image, int16_t H_Image, uint16_t color)
    {
        Adafruit_GFX::drawBitmap(xStart, yStart, image_buffer, W_Image, H_Image, color);
    }
 */

class Paint {
public:
    Paint(unsigned char* image, int16_t width, int16_t height);
    ~Paint();
    void Clear(int colored);
    int GetWidth(void);
    void SetWidth(int width);
    int GetHeight(void);
    void SetHeight(int height);
    int GetRotate(void);
    void SetRotate(int rotate);
    uint8_t getStringWidth(const char* input_str, uint8_t fontW);
    unsigned char* GetImage(void);
    void DrawAbsolutePixel(int x, int y, int colored);
    void drawPixel(int16_t x, int16_t y, uint16_t colored);
    void DrawCharAt(int x, int y, char ascii_char, sFONT* font, int colored);
    void DrawStringAt(int x, int y, const char* text, sFONT* font, int colored);
    void DrawLine(int x0, int y0, int x1, int y1, int colored);
    void DrawHorizontalLine(int x, int y, int width, int colored);
    void DrawVerticalLine(int x, int y, int height, int colored);
    void DrawRectangle(int x0, int y0, int x1, int y1, int colored);
    void DrawFilledRectangle(int x0, int y0, int x1, int y1, int colored);
    void DrawCircle(int x, int y, int radius, int colored);
    void DrawFilledCircle(int x, int y, int radius, int colored);
    void drawInvertedBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color);

private:
    unsigned char* _image;
    uint8_t _rotate;
    uint16_t _width;
    uint16_t _height;
};

#endif

/* END OF FILE */
