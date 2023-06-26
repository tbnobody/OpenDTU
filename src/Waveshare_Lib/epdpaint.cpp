/**
 *  @filename   :   epdpaint.cpp
 *  @brief      :   Paint tools
 *  @author     :   Yehui from Waveshare
 *
 *  Copyright (C) Waveshare     September 9 2017
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

#include "Waveshare_Lib/epdpaint.h"
#if defined(__AVR__) || defined(ARDUINO_ARCH_SAMD)
#include <avr/pgmspace.h>
#elif defined(ESP8266) || defined(ESP32)
#include <pgmspace.h>
#endif

Paint::Paint(unsigned char* image, int16_t width, int16_t height)
{
    this->_rotate = ROTATE_0;
    this->_image = image;
    // 1 byte = 8 pixels, so the width should be the multiple of 8
    this->_width = width % 8 ? width + 8 - (width % 8) : width;
    this->_height = height;
}

Paint::~Paint()
{
}

/**
 *  @brief: clear the image
 */
void Paint::Clear(int colored)
{
    for (int x = 0; x < this->_width; x++) {
        for (int y = 0; y < this->_height; y++) {
            DrawAbsolutePixel(x, y, colored);
        }
    }
}

/**
 *  @brief: this draws a pixel by absolute coordinates.
 *          this function won't be affected by the rotate parameter.
 */
void Paint::DrawAbsolutePixel(int x, int y, int colored)
{
    if (x < 0 || x >= this->_width || y < 0 || y >= this->_height) {
        return;
    }
    if (IF_INVERT_COLOR) {
        if (colored) {
            _image[(x + y * this->_width) / 8] |= 0x80 >> (x % 8);
        } else {
            _image[(x + y * this->_width) / 8] &= ~(0x80 >> (x % 8));
        }
    } else {
        if (colored) {
            _image[(x + y * this->_width) / 8] &= ~(0x80 >> (x % 8));
        } else {
            _image[(x + y * this->_width) / 8] |= 0x80 >> (x % 8);
        }
    }
}

/**
 *  @brief: Getters and Setters
 */
unsigned char* Paint::GetImage(void)
{
    return this->_image;
}

int Paint::GetWidth(void)
{
    return this->_width;
}

void Paint::SetWidth(int width)
{
    this->_width = width % 8 ? width + 8 - (width % 8) : width;
}

int Paint::GetHeight(void)
{
    return this->_height;
}

void Paint::SetHeight(int height)
{
    this->_height = height;
}

int Paint::GetRotate(void)
{
    return this->_rotate;
}

void Paint::SetRotate(int rotate)
{
    this->_rotate = rotate;
}

uint8_t Paint::getStringWidth(const char* input_str, uint8_t fontW)
{
    int count = 0;
    for (const char* p = input_str; *p; ++p) {
        count++;
    }
    return count * fontW;
}

/**
 *  @brief: this draws a pixel by the coordinates
 */

void Paint::drawPixel(int16_t x, int16_t y, uint16_t colored)
{
    int point_temp;
    if (this->_rotate == ROTATE_0) {
        if (x < 0 || x >= this->_width || y < 0 || y >= this->_height) {
            return;
        }
        DrawAbsolutePixel(x, y, colored);
    } else if (this->_rotate == ROTATE_90) {
        if (x < 0 || x >= this->_height || y < 0 || y >= this->_width) {
            return;
        }
        point_temp = x;
        x = this->_width - y;
        y = point_temp;
        DrawAbsolutePixel(x, y, colored);
    } else if (this->_rotate == ROTATE_180) {
        if (x < 0 || x >= this->_width || y < 0 || y >= this->_height) {
            return;
        }
        x = this->_width - x;
        y = this->_height - y;
        DrawAbsolutePixel(x, y, colored);
    } else if (this->_rotate == ROTATE_270) {
        if (x < 0 || x >= this->_height || y < 0 || y >= this->_width) {
            return;
        }
        point_temp = x;
        x = y;
        y = this->_height - point_temp;
        DrawAbsolutePixel(x, y, colored);
    }
}

/**
 *  @brief: this draws a charactor on the frame buffer but not refresh
 */
void Paint::DrawCharAt(int x, int y, char ascii_char, sFONT* font, int colored)
{
    int i, j;
    unsigned int char_offset = (ascii_char - ' ') * font->Height * (font->Width / 8 + (font->Width % 8 ? 1 : 0));
    const unsigned char* ptr = &font->table[char_offset];

    for (j = 0; j < font->Height; j++) {
        for (i = 0; i < font->Width; i++) {
            if (pgm_read_byte(ptr) & (0x80 >> (i % 8))) {
                drawPixel(x + i, y + j, colored);
            }
            if (i % 8 == 7) {
                ptr++;
            }
        }
        if (font->Width % 8 != 0) {
            ptr++;
        }
    }
}

/**
 *  @brief: this displays a string on the frame buffer but not refresh
 */
void Paint::DrawStringAt(int x, int y, const char* text, sFONT* font, int colored)
{
    const char* p_text = text;
    unsigned int counter = 0;
    int refcolumn = x;

    /* Send the string character by character on EPD */
    while (*p_text != 0) {
        /* Display one character on EPD */
        DrawCharAt(refcolumn, y, *p_text, font, colored);
        /* Decrement the column position by 16 */
        refcolumn += font->Width;
        /* Point on the next character */
        p_text++;
        counter++;
    }
}

/**
 *  @brief: this draws a line on the frame buffer
 */
void Paint::DrawLine(int x0, int y0, int x1, int y1, int colored)
{
    /* Bresenham algorithm */
    int dx = x1 - x0 >= 0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 - y0 <= 0 ? y1 - y0 : y0 - y1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while ((x0 != x1) && (y0 != y1)) {
        drawPixel(x0, y0, colored);
        if (2 * err >= dy) {
            err += dy;
            x0 += sx;
        }
        if (2 * err <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/**
 *  @brief: this draws a horizontal line on the frame buffer
 */
void Paint::DrawHorizontalLine(int x, int y, int line_width, int colored)
{
    int i;
    for (i = x; i < x + line_width; i++) {
        drawPixel(i, y, colored);
    }
}

/**
 *  @brief: this draws a vertical line on the frame buffer
 */
void Paint::DrawVerticalLine(int x, int y, int line_height, int colored)
{
    int i;
    for (i = y; i < y + line_height; i++) {
        drawPixel(x, i, colored);
    }
}

/**
 *  @brief: this draws a rectangle
 */
void Paint::DrawRectangle(int x0, int y0, int x1, int y1, int colored)
{
    int min_x, min_y, max_x, max_y;
    min_x = x1 > x0 ? x0 : x1;
    max_x = x1 > x0 ? x1 : x0;
    min_y = y1 > y0 ? y0 : y1;
    max_y = y1 > y0 ? y1 : y0;

    DrawHorizontalLine(min_x, min_y, max_x - min_x + 1, colored);
    DrawHorizontalLine(min_x, max_y, max_x - min_x + 1, colored);
    DrawVerticalLine(min_x, min_y, max_y - min_y + 1, colored);
    DrawVerticalLine(max_x, min_y, max_y - min_y + 1, colored);
}

/**
 *  @brief: this draws a filled rectangle
 */
void Paint::DrawFilledRectangle(int x0, int y0, int x1, int y1, int colored)
{
    int min_x, min_y, max_x, max_y;
    int i;
    min_x = x1 > x0 ? x0 : x1;
    max_x = x1 > x0 ? x1 : x0;
    min_y = y1 > y0 ? y0 : y1;
    max_y = y1 > y0 ? y1 : y0;

    for (i = min_x; i <= max_x; i++) {
        DrawVerticalLine(i, min_y, max_y - min_y + 1, colored);
    }
}

/**
 *  @brief: this draws a circle
 */
void Paint::DrawCircle(int x, int y, int radius, int colored)
{
    /* Bresenham algorithm */
    int x_pos = -radius;
    int y_pos = 0;
    int err = 2 - 2 * radius;
    int e2;

    do {
        drawPixel(x - x_pos, y + y_pos, colored);
        drawPixel(x + x_pos, y + y_pos, colored);
        drawPixel(x + x_pos, y - y_pos, colored);
        drawPixel(x - x_pos, y - y_pos, colored);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if (-x_pos == y_pos && e2 <= x_pos) {
                e2 = 0;
            }
        }
        if (e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while (x_pos <= 0);
}

/**
 *  @brief: this draws a filled circle
 */
void Paint::DrawFilledCircle(int x, int y, int radius, int colored)
{
    /* Bresenham algorithm */
    int x_pos = -radius;
    int y_pos = 0;
    int err = 2 - 2 * radius;
    int e2;

    do {
        drawPixel(x - x_pos, y + y_pos, colored);
        drawPixel(x + x_pos, y + y_pos, colored);
        drawPixel(x + x_pos, y - y_pos, colored);
        drawPixel(x - x_pos, y - y_pos, colored);
        DrawHorizontalLine(x + x_pos, y + y_pos, 2 * (-x_pos) + 1, colored);
        DrawHorizontalLine(x + x_pos, y - y_pos, 2 * (-x_pos) + 1, colored);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if (-x_pos == y_pos && e2 <= x_pos) {
                e2 = 0;
            }
        }
        if (e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while (x_pos <= 0);
}

void Paint::drawInvertedBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color)
{
    // taken from Adafruit_GFX.cpp, modified
    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            if (i & 7)
                byte <<= 1;
            else {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
                byte = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
#else
                byte = bitmap[j * byteWidth + i / 8];
#endif
            }
            if (!(byte & 0x80)) {
                drawPixel(x + i, y + j, color);
            }
        }
    }
}

/* END OF FILE */