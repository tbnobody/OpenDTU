/*  ---------------------------------------------------------------------------
    File: cpp
    Author(s):  Allen Kempe <allenck@windstream.net>
    Date Created: 05/14/2019
    Last modified: 05/14/2019
    Description: 
    This library can drive led strips through the RMT module on the ESP32.
    GITHUB: https://github.com/allenck/ESP32_led_strip/

    Modified for OpenDTU integration
    Author(s): Florent Carlier <tifaifai.maupiti@gmail.com>
    Date Created: 07/25/2023
    Last modified: 07/25/2023
    Description:
    Coding style of OpenDTU with different effect on the ESP32-S3 board 
    ------------------------------------------------------------------------ */
#include "Led_Strip.h"
#include "Configuration.h"
#include "Datastore.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "PinMapping.h"
#include <Hoymiles.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

#define LED_STRIP_TASK_SIZE             (4096)
#define LED_STRIP_TASK_PRIORITY         (configMAX_PRIORITIES - 1)

#define LED_STRIP_REFRESH_PERIOD_MS     (30U) // TODO: add as parameter to led_strip_init

#define LED_STRIP_NUM_RMT_ITEMS_PER_LED (24U) // Assumes 24 bit color for each led

// RMT Clock source is @ 80 MHz. Dividing it by 8 gives us 10 MHz frequency, or 100ns period.
#define LED_STRIP_RMT_CLK_DIV (8)

Color::Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t white)
{
 val = ((uint32_t)white << 24) | ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
}
 Color::Color(uint32_t c) {}
 uint32_t Color::value() {return val;}
 uint8_t Color::red() {return (val&0xff0000) >> 16;}
 uint8_t Color::green() {return (val&0xff00) >> 8;}
 uint8_t Color::blue() {return val&0xff;}
 uint8_t Color::white() {return (val&0xff000000) >> 24;}

/* OpenDTU Always defined : 
clock_t millis()
{
 clock_t t = clock() / (CLOCKS_PER_SEC / 1000);
 return t;
}*/

LedStripClass LEDStrip;

/*
 * Constructor when length, pin and type are known at compile-time. 
*/
LedStripClass::LedStripClass()
{
}

LedStripClass::~LedStripClass()
{
}

bool LedStripClass::init()
{
  auto& pin = PinMapping.get();

  _numPixels = 1; // CONFIG_LED_STRIP_NUM_PIXELS = 1
  gpio = pin.led_rgb; // CONFIG_LED_STRIP_GPIO_PIN = 48
  neoPixelType = NEO_GRB + NEO_KHZ800;
  rmtChannel = 0; // CONFIG_RMT_CHANNEL

  _updateRGBTimeout.set(LEDSTRIP_UPDATE_INTERVAL);
  if (pin.led_rgb >= 0) {
    _ledRGBActive = 1;
    _ledRGBState[0] = LedRGBState_t::OffN;
    _ledRGBState[1] = LedRGBState_t::OffI;
  }

 	//uint32_t b1[_numPixels];
  //buf1 = b1; 
  buf1 = (uint32_t*) malloc(sizeof(uint32_t)* _numPixels);
  //uint32_t b2[_numPixels];
  //buf2 = b2;
  buf2 = (uint32_t*) malloc(sizeof(uint32_t)* _numPixels);
  showingBuf1 = false;
  access_semaphore = xSemaphoreCreateBinary();
	wOffset = (neoPixelType >> 6) & 0b11; // See notes in header file
  rOffset = (neoPixelType >> 4) & 0b11; // regarding R/G/B/W offsets
  gOffset = (neoPixelType >> 2) & 0b11;
	bOffset = neoPixelType & 0b11;
  hasWhite = (wOffset != rOffset);
  memset(buf1, 0, sizeof(uint32_t)* _numPixels);
  memset(buf2, 0, sizeof(uint32_t)* _numPixels);

  if(!init_rmt())
   return false;

  xSemaphoreGive(access_semaphore);
  
  BaseType_t task_created = xTaskCreate(led_strip_task,
                                        "led_strip_task",
                                        LED_STRIP_TASK_SIZE,
                                        this,
                                        +6, //LED_STRIP_TASK_PRIORITY,
                                        &led_strip_task_handle
                                     );

  if (!task_created) {
      return false;
  }
  return true;
}

bool LedStripClass::clearOnShow() {return bClearOnShow;}
void LedStripClass::setClearOnShow(bool b) {bClearOnShow = b;}

bool LedStripClass::init_rmt()
{
		rmt_config_t rmt_cfg;
		rmt_cfg.rmt_mode =RMT_MODE_TX;							// rmt_mode_t rmt_mode
		rmt_cfg.channel = (rmt_channel_t)rmtChannel;// rmt_channel_t channel
		rmt_cfg.clk_div = LED_STRIP_RMT_CLK_DIV;		// uint8_t clk_div
		rmt_cfg.gpio_num =(gpio_num_t)gpio;					// gpio_num_t gpio_num
		rmt_cfg.mem_block_num = 1;												// mem_block_num
		//struct rmt_tx_config_t
		rmt_cfg.tx_config.loop_en = false; 								// bool loop_en
		rmt_cfg.tx_config.carrier_freq_hz =  100;					// uint32_t carrier_freq_hz Not used, but has to be set to avoid divide by 0 err
		rmt_cfg.tx_config.carrier_duty_percent = 50;			// uint8_t carrier_duty_percent
		rmt_cfg.tx_config.carrier_level = RMT_CARRIER_LEVEL_LOW; // rmt_carrier_level_t carrier_level
		rmt_cfg.tx_config.carrier_en = false;							// bool carrier_en
		rmt_cfg.tx_config.idle_level= RMT_IDLE_LEVEL_LOW; // rmt_idle_level_t idle_level
		rmt_cfg.tx_config.idle_output_en = true;					// bool idle_output_en
		 
    esp_err_t cfg_ok = rmt_config(&rmt_cfg);
    if (cfg_ok != ESP_OK) {
        return false;
    }
#if 1
    esp_err_t install_ok = rmt_driver_install(rmt_cfg.channel, 0, 0);
    if (install_ok != ESP_OK) {
     return false;
    }
#endif
    return true;
}

bool LedStripClass::show()
{
	bool success = true;

  xSemaphoreTake(access_semaphore, portMAX_DELAY);
  if (showingBuf1) {
    showingBuf1 = false;
 		if(bClearOnShow)
    	memset(buf1, 0, sizeof(uint32_t) * _numPixels);
    else
    {
     memcpy(buf1, buf2, sizeof(uint32_t) * _numPixels);
		}  
  } 
  else {
    showingBuf1 = true;
    if(bClearOnShow)
			memset(buf2, 0, sizeof(uint32_t) * _numPixels);
  	else
    {
     memcpy(buf2, buf1, sizeof(uint32_t) * _numPixels);
		}
  }
	xSemaphoreGive(access_semaphore);

  return success;
}

void LedStripClass::led_strip_task(void *arg)
{
  LedStripClass* strip = (LedStripClass*)arg;  
    
  led_fill_rmt_items_fn led_make_waveform = NULL;
  
    bool make_new_rmt_items = true;
    bool prev_showing_buf_1 = !strip->showingBuf1;

    size_t num_items_malloc = (LED_STRIP_NUM_RMT_ITEMS_PER_LED * strip->_numPixels);
    rmt_item32_t *rmt_items = (rmt_item32_t*) malloc(sizeof(rmt_item32_t) * num_items_malloc);
    if (!rmt_items) {
        vTaskDelete(NULL);
    }
#if 0
    switch (led_strip->rgb_led_type) {
        case RGB_LED_TYPE_WS2812:
            led_make_waveform = led_strip_fill_rmt_items_ws2812;
            break;

        default:
            // Will avoid keeping it point to NULL
            led_make_waveform = led_strip_fill_rmt_items_ws2812;
            break;
    };
#else
		led_make_waveform = led_strip_fill_rmt_items_ws2812;
#endif
    for(;;) 
		{
        rmt_wait_tx_done((rmt_channel_t)strip->rmtChannel, portMAX_DELAY);
        xSemaphoreTake(strip->access_semaphore, portMAX_DELAY);

        /*
         * If buf 1 was previously being shown and now buf 2 is being shown,
         * it should update the new rmt items array. If buf 2 was previous being shown
         * and now buf 1 is being shown, it should update the new rmt items array.
         * Otherwise, no need to update the array
         */
        if ((prev_showing_buf_1 == true) && (strip->showingBuf1 == false)) {
            make_new_rmt_items = true;
        } else if ((prev_showing_buf_1 == false) && (strip->showingBuf1 == true)) {
            make_new_rmt_items = true;
        } else {
            make_new_rmt_items = false;
        }

        if (make_new_rmt_items) {
            if (strip->showingBuf1) {
                led_make_waveform(strip->buf1, rmt_items, strip->_numPixels,
									strip->wOffset, strip->rOffset, strip->gOffset, strip->bOffset);
            } else {
                led_make_waveform(strip->buf2, rmt_items, strip->_numPixels,
									strip->wOffset, strip->rOffset, strip->gOffset, strip->bOffset);
            }
        }

        rmt_write_items((rmt_channel_t)strip->rmtChannel, rmt_items, num_items_malloc, false);
        prev_showing_buf_1 = strip->showingBuf1;
        xSemaphoreGive(strip->access_semaphore);
        vTaskDelay(LED_STRIP_REFRESH_PERIOD_MS / portTICK_PERIOD_MS);
    }

    if (rmt_items) {
        free(rmt_items);
    }
    vTaskDelete(NULL);
}

void LedStripClass::led_strip_fill_rmt_items_ws2812(uint32_t *led_strip_buf, rmt_item32_t *rmt_items, uint32_t led_strip_length, uint8_t wOffset, uint8_t rOffset,uint8_t gOffset,uint8_t bOffset)
{
		
    uint32_t rmt_items_index = 0;
    
    
    for (uint32_t led_index = 0; led_index < led_strip_length; led_index++) {
        uint32_t led_color = led_strip_buf[led_index];
        uint8_t bytes[4]= {(uint8_t)((led_color&0xff0000)>>16), (uint8_t)((led_color&0xff00)>>8), (uint8_t)(led_color&0xff), (uint8_t)((led_color&0xff)>>24)};
        for (uint8_t bit = 8; bit != 0; bit--) {
            uint8_t bit_set = (bytes[wOffset] >> (bit - 1)) & 1;
            if(bit_set) {
                LedStripClass::led_strip_rmt_bit_1_ws2812(&(rmt_items[rmt_items_index]));
            } else {
                LedStripClass::led_strip_rmt_bit_0_ws2812(&(rmt_items[rmt_items_index]));
            }
            rmt_items_index++;
        }
        for (uint8_t bit = 8; bit != 0; bit--) {
            uint8_t bit_set = (bytes[gOffset] >> (bit - 1)) & 1;
            if(bit_set) {
                LedStripClass::led_strip_rmt_bit_1_ws2812(&(rmt_items[rmt_items_index]));
            } else {
                LedStripClass::led_strip_rmt_bit_0_ws2812(&(rmt_items[rmt_items_index]));
            }
            rmt_items_index++;
        }
        for (uint8_t bit = 8; bit != 0; bit--) {
            uint8_t bit_set = (bytes[bOffset] >> (bit - 1)) & 1;
            if(bit_set) {
                led_strip_rmt_bit_1_ws2812(&(rmt_items[rmt_items_index]));
            } else {
                led_strip_rmt_bit_0_ws2812(&(rmt_items[rmt_items_index]));
            }
            rmt_items_index++;
        }
        if(wOffset != rOffset)
        { 
		      for (uint8_t bit = 8; bit != 0; bit--) {
		          uint8_t bit_set = (bytes[wOffset] >> (bit - 1)) & 1;
		          if(bit_set) {
		              led_strip_rmt_bit_1_ws2812(&(rmt_items[rmt_items_index]));
		          } else {
		              led_strip_rmt_bit_0_ws2812(&(rmt_items[rmt_items_index]));
		          }
		          rmt_items_index++;
		      }
       }
    }
}

bool LedStripClass::setPixelColor(uint32_t pixel_num, uint32_t  color)
{
    bool set_led_success = true;

    if (showingBuf1) {
        buf2[pixel_num] = color;
    } else {
        buf1[pixel_num] = color;
    }

    return set_led_success;
}

/**
 * Clears the LED strip
 */
bool LedStripClass::clear()
{
    bool success = true;

    if (showingBuf1) {
        memset(buf2, 0, sizeof(uint32_t) * _numPixels);
    } else {
        memset(buf1, 0, sizeof(uint32_t) * _numPixels);
    }

    return success;
}

bool LedStripClass::getPixelColor(uint32_t pixel_num, uint32_t color)
{
    bool get_success = true;

    if ((pixel_num > _numPixels) ||
        (!color)) {
        color = 0;
        return false;
    }

    if (showingBuf1) {
        color = buf1[pixel_num];
    } else {
        color = buf2[pixel_num];
    }

    return get_success;
}

/*!
    @brief   An 8-bit gamma-correction function for basic pixel brightness
             adjustment. Makes color transitions appear more perceptially
             correct.
    @param   x  Input brightness, 0 (minimum or off/black) to 255 (maximum).
    @return  Gamma-adjusted brightness, can then be passed to one of the
             setPixelColor() functions. This uses a fixed gamma correction
             exponent of 2.6, which seems reasonably okay for average
             NeoPixels in average tasks. If you need finer control you'll
             need to provide your own gamma-correction function instead.
  */
uint8_t LedStripClass::gamma8(uint8_t x) {
    return /*pgm_read_byte*/(_NeoPixelGammaTable[x]); // 0-255 in, 0-255 out
}

/*!
  @brief   Fill all or part of the NeoPixel strip with a color.
  @param   c      32-bit color value. Most significant byte is white (for
                  RGBW pixels) or ignored (for RGB pixels), next is red,
                  then green, and least significant byte is blue. If all
                  arguments are unspecified, this will be 0 (off).
  @param   first  Index of first pixel to fill, starting from 0. Must be
                  in-bounds, no clipping is performed. 0 if unspecified.
  @param   count  Number of pixels to fill, as a positive value. Passing
                  0 or leaving unspecified will fill to end of 
*/
void LedStripClass::fill(uint32_t c, uint16_t first, uint16_t count) {
  uint16_t i, end;

  if(first >= _numPixels) {
    return; // If first LED is past end of strip, nothing to do
  }

  // Calculate the index ONE AFTER the last pixel to fill
  if(count == 0) {
    // Fill to end of strip
    end = _numPixels;
  } else {
    // Ensure that the loop won't go past the last pixel
    end = first + count;
    if(end > _numPixels) end = _numPixels;
  }

  
  for(i = first; i < end; i++) {
   setPixelColor(i, c);
  }
}

uint32_t LedStripClass::ColorHSV(uint16_t hue, uint8_t sat, uint8_t val) {

 uint8_t r, g, b;
 // Remap 0-65535 to 0-1529. Pure red is CENTERED on the 64K rollover;
  // 0 is not the start of pure red, but the midpoint...a few values above
  // zero and a few below 65536 all yield pure red (similarly, 32768 is the
  // midpoint, not start, of pure cyan). The 8-bit RGB hexcone (256 values
  // each for red, green, blue) really only allows for 1530 distinct hues
  // (not 1536, more on that below), but the full unsigned 16-bit type was
  // chosen for hue so that one's code can easily handle a contiguous color
  // wheel by allowing hue to roll over in either direction.
  hue = (hue * 1530L + 32768) / 65536;
  // Because red is centered on the rollover point (the +32768 above,
  // essentially a fixed-point +0.5), the above actually yields 0 to 1530,
  // where 0 and 1530 would yield the same thing. Rather than apply a
  // costly modulo operator, 1530 is handled as a special case below.

  // So you'd think that the color "hexcone" (the thing that ramps from
  // pure red, to pure yellow, to pure green and so forth back to red,
  // yielding six slices), and with each color component having 256
  // possible values (0-255), might have 1536 possible items (6*256),
  // but in reality there's 1530. This is because the last element in
  // each 256-element slice is equal to the first element of the next
  // slice, and keeping those in there this would create small
  // discontinuities in the color wheel. So the last element of each
  // slice is dropped...we regard only elements 0-254, with item 255
  // being picked up as element 0 of the next slice. Like this:
  // Red to not-quite-pure-yellow is:        255,   0, 0 to 255, 254,   0
  // Pure yellow to not-quite-pure-green is: 255, 255, 0 to   1, 255,   0
  // Pure green to not-quite-pure-cyan is:     0, 255, 0 to   0, 255, 254
  // and so forth. Hence, 1530 distinct hues (0 to 1529), and hence why
  // the constants below are not the multiples of 256 you might expect.

  // Convert hue to R,G,B (nested ifs faster than divide+mod+switch):
  if(hue < 510) {         // Red to Green-1
    b = 0;
    if(hue < 255) {       //   Red to Yellow-1
      r = 255;
      g = hue;            //     g = 0 to 254
    } else {              //   Yellow to Green-1
      r = 510 - hue;      //     r = 255 to 1
      g = 255;
    }
  } else if(hue < 1020) { // Green to Blue-1
    r = 0;
    if(hue <  765) {      //   Green to Cyan-1
      g = 255;
      b = hue - 510;      //     b = 0 to 254
    } else {              //   Cyan to Blue-1
      g = 1020 - hue;     //     g = 255 to 1
      b = 255;
    }
  } else if(hue < 1530) { // Blue to Red-1
    g = 0;
    if(hue < 1275) {      //   Blue to Magenta-1
      r = hue - 1020;     //     r = 0 to 254
      b = 255;
    } else {              //   Magenta to Red-1
      r = 255;
      b = 1530 - hue;     //     b = 255 to 1
    }
  } else {                // Last 0.5 Red (quicker than % operator)
    r = 255;
    g = b = 0;
  }

  // Apply saturation and value to R,G,B, pack into 32-bit result:
  uint32_t v1 =   1 + val; // 1 to 256; allows >>8 instead of /255
  uint16_t s1 =   1 + sat; // 1 to 256; same reason
  uint8_t  s2 = 255 - sat; // 255 to 0
  return ((((((r * s1) >> 8) + s2) * v1) & 0xff00) << 8) |
          (((((g * s1) >> 8) + s2) * v1) & 0xff00)       |
         ( ((((b * s1) >> 8) + s2) * v1)           >> 8);
}

// A 32-bit variant of gamma8() that applies the same function
// to all components of a packed RGB or WRGB value.
uint32_t LedStripClass::gamma32(uint32_t x) {
  uint8_t *y = (uint8_t *)&x;
  // All four bytes of a 32-bit value are filtered even if RGB (not WRGB),
  // to avoid a bunch of shifting and masking that would be necessary for
  // properly handling different endianisms (and each byte is a fairly
  // trivial operation, so it might not even be wasting cycles vs a check
  // and branch for the RGB case). In theory this might cause trouble *if*
  // someone's storing information in the unused most significant byte

  // of an RGB value, but this seems exceedingly rare and if it's
  // encountered in reality they can mask values going in or coming out.
  for(uint8_t i=0; i<4; i++) y[i] = gamma8(y[i]);
  return x; // Packed 32-bit return
}

// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void LedStripClass::colorWipe(uint32_t color, int wait) {
  for(int i=0; i<numPixels(); i++) { // For each pixel in ..
    setPixelColor(i, color);         //  Set pixel's color (in RAM)
    show();                          //  Update strip to match
    delay(wait);                     //  Pause for a moment
  }
}

void LedStripClass::whiteOverRainbow(int whiteSpeed, int whiteLength) {
  if(whiteLength >= numPixels()) whiteLength = numPixels() - 1;

  int      head          = whiteLength - 1;
  int      tail          = 0;
  int      loops         = 3;
  int      loopNum       = 0;
  clock_t lastTime      = millis();
  uint32_t firstPixelHue = 0;

  for(;;) 
  { // Repeat forever (or until a 'break' or 'return')
    for(int i=0; i<numPixels(); i++) 
    {  // For each pixel in ..
      if(((i >= tail) && (i <= head)) ||      //  If between head & tail...
         ((tail > head) && ((i >= tail) || (i <= head))))
      {
        setPixelColor(i, Color(0,0,0,255).value()); // Set white
      } else {                                             // else set rainbow
        int pixelHue = firstPixelHue + (i * 65536L / numPixels());
        uint32_t c = gamma32(ColorHSV(pixelHue, 255, 255));
        setPixelColor(i, c);
      }
    }

    show(); // Update strip with new contents
    // There's no delay here, it just runs full-tilt until the timer and
    // counter combination below runs out.

    firstPixelHue += 40; // Advance just a little along the color wheel
    if(((uint32_t)time(NULL) - lastTime) > whiteSpeed) 
    { // Time to update head/tail?
      if(++head >= numPixels()) 
      {      // Advance head, wrap around
        head = 0;
        if(++loopNum >= loops) return;
      }
      if(++tail >= numPixels()) 
      {      // Advance tail, wrap around
        tail = 0;
      }
      lastTime = millis();                   // Save time of last movement
    }
  }
}

void LedStripClass::pulseWhite(uint8_t wait) {
  for(int j=0; j<256; j++) { // Ramp up from 0 to 255
    // Fill entire strip with white at gamma-corrected brightness level 'j':
    fill(Color(gamma8(j),gamma8(j),gamma8(j)).value(),0,0);
    show();
    delay(wait);
  }
  for(int j=255; j>=0; j--) { // Ramp down from 255 to 0
        fill(Color(gamma8(j),gamma8(j),gamma8(j)).value(),0,0);
    show();
    delay(wait);
  }
}

void LedStripClass::rainbowFade2White(int wait, int rainbowLoops, int whiteLoops) {
  int fadeVal=0, fadeMax=100;

  // Hue of first pixel runs 'rainbowLoops' complete loops through the color
  // wheel. Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to rainbowLoops*65536, using steps of 256 so we
  // advance around the wheel at a decent clip.
  for(uint32_t firstPixelHue = 0; firstPixelHue < rainbowLoops*65536;
    firstPixelHue += 256) {

    for(int i=0; i<numPixels(); i++) { // For each pixel in ..

      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (numPixels() steps):
      uint32_t pixelHue = firstPixelHue + (i * 65536L / numPixels());

      // ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the three-argument variant, though the
      // second value (saturation) is a constant 255.
      uint32_t c = gamma32(ColorHSV(pixelHue, 255, 255* fadeVal / fadeMax));
      setPixelColor(i, c);
    }

    show();
    delay(wait);

    if(firstPixelHue < 65536) {                              // First loop,
      if(fadeVal < fadeMax) fadeVal++;                       // fade in
    } else if(firstPixelHue >= ((rainbowLoops-1) * 65536)) { // Last loop,
      if(fadeVal > 0) fadeVal--;                             // fade out
    } else {
      fadeVal = fadeMax; // Interim loop, make sure fade is at max
    }
  }

  for(int k=0; k<whiteLoops; k++) {
    for(int j=0; j<256; j++) { // Ramp up 0 to 255
      // Fill entire strip with white at gamma-corrected brightness level 'j':
 			fill(Color(gamma8(j),gamma8(j),gamma8(j)).value(),0,0);
      show();
    }
    delay(1000); // Pause 1 second
    for(int j=255; j>=0; j--) { // Ramp down 255 to 0
      fill(Color(gamma8(j),gamma8(j),gamma8(j)).value(),0,0);
      show();
    }
  }

  delay(500); // Pause 1/2 second
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void LedStripClass::theaterChase(uint32_t color, int wait) {
  for(int a=0; a<10; a++) {  // Repeat 10 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for(int c=b; c<numPixels(); c += 3) 
			{
        setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      show(); // Update strip with new contents
      delay(wait);  // Pause for a moment
    }
  }
}

// Rainbow cycle along whole  Pass delay time (in ms) between frames.
void LedStripClass::rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<numPixels(); i++) { // For each pixel in ..
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / numPixels());
      // ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through gamma32() to provide 'truer' colors
      // before assigning to each pixel:
			uint32_t color = gamma32(ColorHSV(pixelHue, 255, 255));
      setPixelColor(i, color);
    }
    show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void LedStripClass::theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for(int a=0; a<30; a++) {  // Repeat 30 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for(int c=b; c<numPixels(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (numPixels() steps):
        int      hue   = firstPixelHue + c * 65536L / numPixels();
        uint32_t color = gamma32(ColorHSV(hue, 255, 255)); // hue -> RGB
				setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      show();                // Update strip with new contents
      delay(wait);                 // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}

void LedStripClass::loop_example() {
  if (_ledRGBActive == 0) {
        return;
  }

  colorWipe(Color(255, 0, 0).value(), 50);     // RED
  colorWipe(Color(0, 255, 0).value(), 50);     // GREEN
	colorWipe(Color(0, 0, 255).value(), 50);     // BLUE
  colorWipe(Color(255, 255, 0).value(), 50);   // YELLOW
  colorWipe(Color(255, 51, 204).value(), 50);  // VIOLET
  colorWipe(Color(255, 102, 0).value(), 50);   // ORANGE

  
  whiteOverRainbow(75, 5);

  pulseWhite( 5);

  // ?: rainbowFade2White(3, 3, 1);

  // Do a theater marquee effect in various colors...
  theaterChase(Color(0,127, 127, 127).value(), 50); // White, half brightness
  theaterChase(Color(0, 127, 0, 0).value(), 50); // Red, half brightness
  theaterChase(Color(0, 0, 0, 127).value(), 50); // Blue, half brightness

  rainbow(10);             // Flowing rainbow cycle along the whole strip
  theaterChaseRainbow(50); // Rainbow-enhanced theaterChase variant
}

void LedStripClass::loop() {
  if (_ledRGBActive == 0) {
        return;
  }

  if (_updateRGBTimeout.occured()) {
        const CONFIG_T& config = Configuration.get();

        // Update network status
        _ledRGBState[0] = LedRGBState_t::OffN;

        if (NetworkSettings.isConnected()) {
            _ledRGBState[0] = LedRGBState_t::BlinkN;
        }

        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 5) && (!config.Mqtt_Enabled || (config.Mqtt_Enabled && MqttSettings.getConnected()))) {
            _ledRGBState[0] = LedRGBState_t::OnN;
        }

        // Update inverter status
        _ledRGBState[1] = LedRGBState_t::OffI;
        if (Hoymiles.getNumInverters() && Datastore.getIsAtLeastOnePollEnabled()) {
            // set LED status
            if (Datastore.getIsAllEnabledReachable() && Datastore.getIsAllEnabledProducing()) {
                _ledRGBState[1] = LedRGBState_t::OnI;
            }
            if (Datastore.getIsAllEnabledReachable() && !Datastore.getIsAllEnabledProducing()) {
                _ledRGBState[1] = LedRGBState_t::BlinkI;
            }
        }

        _updateRGBTimeout.reset();
    }

  for (uint8_t i = 0; i < 2; i++) {
    switch (_ledRGBState[i]) {
        case LedRGBState_t::OffN:
            colorWipe(Color(255, 0, 0).value(), 1000);    // RED: Network is Off
            break;
        case LedRGBState_t::OnN:
            colorWipe(Color(0, 255, 0).value(), 1000);    // GREEN: Network is On
            break;
        case LedRGBState_t::BlinkN:
            colorWipe(Color(255, 51, 204).value(), 1000);  // VIOLET: Network is Connected
            break;
        case LedRGBState_t::OffI:
            colorWipe(Color(0, 0, 255).value(), 1000);    // BLUE: Inverter is Off            
            break;
        case LedRGBState_t::OnI:
            colorWipe(Color(255, 255, 0).value(), 1000);  // YELLOW: Inverter is On
            break;
        case LedRGBState_t::BlinkI:
            colorWipe(Color(255, 102, 0).value(), 1000);  // ORANGE: Inverter is Blink
            break;
        }
  }

}