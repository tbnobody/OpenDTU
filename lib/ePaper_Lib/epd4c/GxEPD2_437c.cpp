// Display Library for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// based on Demo Example from Waveshare: https://github.com/waveshare/e-Paper/tree/master/Arduino/epd4in37g
// Panel: 4.37inch 4-Color E-Paper : https://www.waveshare.com/product/displays/e-paper/4.37inch-e-paper-module-g.htm
// Controller: unknown
// initcode extracted from Waveshare library file epd4in37g.cpp from: https://github.com/waveshare/e-Paper/tree/master/Arduino/epd4in37g
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2

#include "GxEPD2_437c.h"

GxEPD2_437c::GxEPD2_437c(int16_t cs, int16_t dc, int16_t rst, int16_t busy) :
  GxEPD2_EPD(cs, dc, rst, busy, LOW, 25000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate, hasFastPartialUpdate)
{
  _paged = false;
}

void GxEPD2_437c::clearScreen(uint8_t value)
{
  clearScreen(value, 0xFF);
}

void GxEPD2_437c::clearScreen(uint8_t black_value, uint8_t color_value)
{
  writeScreenBuffer(black_value, color_value);
  _Update_Full();
}

void GxEPD2_437c::writeScreenBuffer(uint8_t value)
{
  writeScreenBuffer(value, 0xFF);
}

void GxEPD2_437c::writeScreenBuffer(uint8_t black_value, uint8_t color_value)
{
  _initial_write = false; // initial full screen buffer clean done
  _Init_Full();
  _writeCommand(0x10);
  _startTransfer();
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 4; i++)
  {
    _transfer(0xFF == black_value ? 0x55 : 0x00);
  }
  _endTransfer();
}

void GxEPD2_437c::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  //Serial.print("writeImage("); Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if (_paged && (x == 0) && (w == int16_t(WIDTH)) && (h < int16_t(HEIGHT)))
  {
    //Serial.println("paged");
    _startTransfer();
    for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(h) / 8; i++)
    {
      uint8_t data = bitmap[i];
      for (int16_t k = 0; k < 2; k++)
      {
        uint8_t data2 = (data & 0x80 ? 0x40 : 0x00) | (data & 0x40 ? 0x10 : 0x00) |
                        (data & 0x20 ? 0x04 : 0x00) | (data & 0x10 ? 0x01 : 0x00);
        data <<= 4;
        _transfer(data2);
      }
    }
    _endTransfer();
    if (y + h == HEIGHT) // last page
    {
      //Serial.println("paged ended");
      _paged = false;
    }
  }
  else
  {
    _paged = false;
    int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
    x -= x % 8; // byte boundary
    w = wb * 8; // byte boundary
    if ((w <= 0) || (h <= 0)) return;
    _Init_Full();
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < int16_t(HEIGHT); i++)
    {
      for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
      {
        uint8_t data = 0xFF;
        if ((j >= x) && (j <= x + w) && (i >= y) && (i < y + h))
        {
          uint32_t idx = mirror_y ? (j - x) / 8 + uint32_t((h - 1 - (i - y))) * wb : (j - x) / 8 + uint32_t(i - y) * wb;
          if (pgm)
          {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
            data = pgm_read_byte(&bitmap[idx]);
#else
            data = bitmap[idx];
#endif
          }
          else
          {
            data = bitmap[idx];
          }
          if (invert) data = ~data;
        }
        for (int16_t k = 0; k < 2; k++)
        {
          uint8_t data2 = (data & 0x80 ? 0x40 : 0x00) | (data & 0x40 ? 0x10 : 0x00) |
                          (data & 0x20 ? 0x04 : 0x00) | (data & 0x10 ? 0x01 : 0x00);
          data <<= 4;
          _transfer(data2);
        }
      }
    }
    _endTransfer();
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_437c::writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (!black && !color) return;
  if (!color) return writeImage(black, x, y, w, h, invert, mirror_y, pgm);
  //Serial.print("writeImage("); Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if (_paged && (x == 0) && (w == int16_t(WIDTH)) && (h < int16_t(HEIGHT)))
  {
    //Serial.println("paged");
    _startTransfer();
    for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(h) / 8; i++)
    {
      uint8_t black_data = black[i];
      uint8_t color_data = color[i];
      for (int16_t k = 0; k < 2; k++)
      {
        uint8_t out_data = 0x00;
        for (int16_t l = 0; l < 4; l++)
        {
          out_data <<= 2;
          if (!(color_data & 0x80)) out_data |= 0x03; // red
          else out_data |= black_data & 0x80 ? 0x01 : 0x00; // white or black
          black_data <<= 1;
          color_data <<= 1;
        }
        _transfer(out_data);
      }
    }
    _endTransfer();
    if (y + h == HEIGHT) // last page
    {
      //Serial.println("paged ended");
      _paged = false;
    }
  }
  else
  {
    _paged = false;
    int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
    x -= x % 8; // byte boundary
    w = wb * 8; // byte boundary
    if ((w <= 0) || (h <= 0)) return;
    _Init_Full();
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < int16_t(HEIGHT); i++)
    {
      for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
      {
        uint8_t black_data = 0xFF, color_data = 0xFF;
        if ((j >= x) && (j < x + w) && (i >= y) && (i < y + h))
        {
          uint32_t idx = mirror_y ? (j - x) / 8 + uint32_t((h - 1 - (i - y))) * wb : (j - x) / 8 + uint32_t(i - y) * wb;
          if (pgm)
          {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
            black_data = pgm_read_byte(&black[idx]);
            color_data = pgm_read_byte(&color[idx]);
#else
            black_data = black[idx];
            color_data = color[idx];
#endif
          }
          else
          {
            black_data = black[idx];
            color_data = color[idx];
          }
          if (invert)
          {
            black_data = ~black_data;
            color_data = ~color_data;
          }
        }
        for (int16_t k = 0; k < 2; k++)
        {
          uint8_t out_data = 0x00;
          for (int16_t l = 0; l < 4; l++)
          {
            out_data <<= 2;
            if (!(color_data & 0x80)) out_data |= 0x03; // red
            else out_data |= black_data & 0x80 ? 0x01 : 0x00; // white or black
            black_data <<= 1;
            color_data <<= 1;
          }
          _transfer(out_data);
        }
      }
    }
    _endTransfer();
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_437c::writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                 int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 7) / 8; // width bytes, bitmaps are padded
  x_part -= x_part % 8; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  x -= x % 8; // byte boundary
  w = 8 * ((w + 7) / 8); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  _Init_Full();
  _writeCommand(0x10);
  _startTransfer();
  for (int16_t i = 0; i < int16_t(HEIGHT); i++)
  {
    for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
    {
      uint8_t data = 0xFF;
      if ((j >= x1) && (j < x1 + w) && (i >= y1) && (i < y1 + h))
      {
        // use wb_bitmap, h_bitmap of bitmap for index!
        uint32_t idx = mirror_y ? (x_part + j - x1) / 8 + uint32_t((h_bitmap - 1 - (y_part + i - y1))) * wb_bitmap : (x_part + j - x1) / 8 + uint32_t(y_part + i - y1) * wb_bitmap;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          data = pgm_read_byte(&bitmap[idx]);
#else
          data = bitmap[idx];
#endif
        }
        else
        {
          data = bitmap[idx];
        }
        if (invert) data = ~data;
      }
      for (int16_t k = 0; k < 2; k++)
      {
        uint8_t data2 = (data & 0x80 ? 0x40 : 0x00) | (data & 0x40 ? 0x10 : 0x00) |
                        (data & 0x20 ? 0x04 : 0x00) | (data & 0x10 ? 0x01 : 0x00);
        data <<= 4;
        _transfer(data2);
      }
    }
  }
  _endTransfer();
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_437c::writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                 int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  //Serial.print("writeImagePart("); Serial.print(x_part); Serial.print(", "); Serial.print(y_part); Serial.print(", ");
  //Serial.print(w_bitmap); Serial.print(", "); Serial.print(h_bitmap); Serial.print(", ");
  //Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  if (!black && !color) return;
  if (!color) return writeImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 7) / 8; // width bytes, bitmaps are padded
  x_part -= x_part % 8; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  x -= x % 8; // byte boundary
  w = 8 * ((w + 7) / 8); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  _Init_Full();
  _writeCommand(0x10);
  _startTransfer();
  for (int16_t i = 0; i < int16_t(HEIGHT); i++)
  {
    for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
    {
      uint8_t black_data = 0xFF, color_data = 0xFF;
      if ((j >= x1) && (j < x1 + w) && (i >= y1) && (i < y1 + h))
      {
        // use wb_bitmap, h_bitmap of bitmap for index!
        uint32_t idx = mirror_y ? (x_part + j - x1) / 8 + uint32_t((h_bitmap - 1 - (y_part + i - y1))) * wb_bitmap : (x_part + j - x1) / 8 + uint32_t(y_part + i - y1) * wb_bitmap;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          black_data = pgm_read_byte(&black[idx]);
          color_data = pgm_read_byte(&color[idx]);
#else
          black_data = black[idx];
          color_data = color[idx];
#endif
        }
        else
        {
          black_data = black[idx];
          color_data = color[idx];
        }
        if (invert)
        {
          black_data = ~black_data;
          color_data = ~color_data;
        }
      }
      for (int16_t k = 0; k < 2; k++)
      {
        uint8_t out_data = 0x00;
        for (int16_t l = 0; l < 4; l++)
        {
          out_data <<= 2;
          if (!(color_data & 0x80)) out_data |= 0x03; // red
          else out_data |= black_data & 0x80 ? 0x01 : 0x00; // white or black
          black_data <<= 1;
          color_data <<= 1;
        }
        _transfer(out_data);
      }
    }
  }
  _endTransfer();
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_437c::writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    //Serial.print("writeNative("); Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
    //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
    delay(1); // yield() to avoid WDT on ESP8266 and ESP32
    if (_paged && (x == 0) && (w == int16_t(WIDTH)) && (h < int16_t(HEIGHT)))
    {
      //Serial.println("paged");
      _startTransfer();
      for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(h) / 4; i++)
      {
        uint8_t data = data1[i];
        _transfer(data);
      }
      _endTransfer();
      if (y + h == HEIGHT) // last page
      {
        //Serial.println("paged ended");
        _paged = false;
      }
    }
    else
    {
      _paged = false;
      int16_t wb = (w + 3) / 4; // width bytes, bitmaps are padded
      x -= x % 4; // byte boundary
      w = wb * 4; // byte boundary
      if ((w <= 0) || (h <= 0)) return;
      _Init_Full();
      _writeCommand(0x10);
      _startTransfer();
      for (int16_t i = 0; i < int16_t(HEIGHT); i++)
      {
        for (int16_t j = 0; j < int16_t(WIDTH); j += 4)
        {
          uint8_t data = 0x55;
          if (data1)
          {
            if ((j >= x) && (j < x + w) && (i >= y) && (i < y + h))
            {
              uint32_t idx = mirror_y ? (j - x) / 4 + uint32_t((h - 1 - (i - y))) * wb : (j - x) / 4 + uint32_t(i - y) * wb;
              if (pgm)
              {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
                data = pgm_read_byte(&data1[idx]);
#else
                data = data1[idx];
#endif
              }
              else
              {
                data = data1[idx];
              }
              if (invert) data = ~data;
            }
          }
          _transfer(data);
        }
      }
      _endTransfer();
    }
    delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  }
}

void GxEPD2_437c::writeNativePart(const uint8_t* data1, const uint8_t* data2, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                  int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  //Serial.print("writeNativePart("); Serial.print(x_part); Serial.print(", "); Serial.print(y_part); Serial.print(", ");
  //Serial.print(w_bitmap); Serial.print(", "); Serial.print(h_bitmap); Serial.print(", ");
  //Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  if (!data1) return;
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 1) / 2; // width bytes, bitmaps are padded
  x_part -= x_part % 2; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  x -= x % 4; // byte boundary
  w = 4 * ((w + 1) / 4); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  _Init_Full();
  _writeCommand(0x10);
  _startTransfer();
  for (int16_t i = 0; i < int16_t(HEIGHT); i++)
  {
    for (int16_t j = 0; j < int16_t(WIDTH); j += 4)
    {
      uint8_t data = 0x55;
      if ((j >= x1) && (j < x1 + w) && (i >= y1) && (i < y1 + h))
      {
        // use wb_bitmap, h_bitmap of bitmap for index!
        uint32_t idx = mirror_y ? (x_part + j - x1) / 4 + uint32_t((h_bitmap - 1 - (y_part + i - y1))) * wb_bitmap : (x_part + j - x1) / 4 + uint32_t(y_part + i - y1) * wb_bitmap;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          data = pgm_read_byte(&data1[idx]);
#else
          data = data1[idx];
#endif
        }
        else
        {
          data = data1[idx];
        }
        if (invert) data = ~data;
      }
      _transfer(data);
    }
  }
  _endTransfer();
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_437c::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_437c::drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_437c::drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(black, color, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_437c::drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(black, color, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_437c::drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeNative(data1, data2, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_437c::refresh(bool partial_update_mode)
{
  if (partial_update_mode) refresh(0, 0, WIDTH, HEIGHT);
  else _Update_Full();
}

void GxEPD2_437c::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
{
  _Update_Part();
}

void GxEPD2_437c::powerOff()
{
  _PowerOff();
}

void GxEPD2_437c::hibernate()
{
  _PowerOff();
  if (_rst >= 0)
  {
    _writeCommand(0x07); // deep sleep
    _writeData(0xA5);    // control code
    _hibernating = true;
  }
}

void GxEPD2_437c::setPaged()
{
  _paged = true;
  _Init_Full();
  _writeCommand(0x10);
}

void GxEPD2_437c::_PowerOn()
{
  if (!_power_is_on)
  {
    _writeCommand(0x04);
    _waitWhileBusy("_PowerOn", power_on_time);
  }
  _power_is_on = true;
}

void GxEPD2_437c::_PowerOff()
{
  if (_power_is_on)
  {
    _writeCommand(0x02);
    _writeData(0x00);
    _waitWhileBusy("_PowerOff", power_off_time);
  }
  _power_is_on = false;
  _using_partial_mode = false;
}

void GxEPD2_437c::_InitDisplay()
{
  if (_hibernating)
  {
    _reset();
    delay(20);
    _reset();
  }
  if (_initial_write) delay(20);
  _writeCommand(0xAA); // CMDH
  _writeData(0x49);
  _writeData(0x55);
  _writeData(0x20);
  _writeData(0x08);
  _writeData(0x09);
  _writeData(0x18);
  _writeCommand(0x01); // Power Settings
  _writeData(0x3F);
  _writeCommand(0x00); // Panel Settings
  _writeData(0x4F);
  _writeData(0x69);
  _writeCommand(0x05); // BTST1
  _writeData(0x40);
  _writeData(0x1F);
  _writeData(0x1F);
  _writeData(0x2C);
  _writeCommand(0x08); // BTST3
  _writeData(0x6F);
  _writeData(0x1F);
  _writeData(0x1F);
  _writeData(0x22);
  _writeCommand(0x06); // Booster Soft Start
  _writeData(0x6F);
  _writeData(0x1F);
  _writeData(0x17);
  _writeData(0x17);
  _writeCommand(0x03); // Power Off Sequence
  _writeData(0x00);
  _writeData(0x54);
  _writeData(0x00);
  _writeData(0x44);
  _writeCommand(0x50); // VCOM and Data Interval Setting
  _writeData(0x3F);    // white border
  _writeCommand(0x60); // TCON
  _writeData(0x02);
  _writeData(0x00);
  _writeCommand(0x30); // PLL Control
  _writeData(0x08);
  _writeCommand(0x61); // Resolution Setting
  _writeData(0x02);
  _writeData(0x00);
  _writeData(0x01);
  _writeData(0x70);
  _writeCommand(0xE3); // PWS
  _writeData(0x2F);
  _writeCommand(0x84); // T_VDCS
  _writeData(0x01);
}

void GxEPD2_437c::_Init_Full()
{
  _InitDisplay();
  _PowerOn();
}

void GxEPD2_437c::_Init_Part()
{
  _InitDisplay();
  _PowerOn();
}

void GxEPD2_437c::_Update_Full()
{
  _writeCommand(0x12); // Display Refresh
  _writeData(0x01);
  delay(1);
  _waitWhileBusy("_Update_Full", full_refresh_time);
}

void GxEPD2_437c::_Update_Part()
{
  _writeCommand(0x12); // Display Refresh
  _writeData(0x01);
  delay(1);
  _waitWhileBusy("_Update_Part", partial_refresh_time);
}
