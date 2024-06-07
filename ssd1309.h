/*
MIT License

Copyright (c) 2021 David Schramm
              2024 Cédric Hirschi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/**
 * @file ssd1309.h
 *
 * simple driver for ssd1309 displays
 */

#ifndef _inc_ssd1309
#define _inc_ssd1309
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

#include "fonts/Adafruit_GFX.h"

typedef enum
{
	SSD1309_PIN_DC,
	SSD1309_PIN_CS,
	SSD1309_PIN_RST
} ssd1309_pin_t;

typedef bool (*ssd1309_spi_callback_t)(uint8_t *data, size_t len);
typedef bool (*ssd1309_pin_callback_t)(ssd1309_pin_t pin, bool state);
typedef void (*ssd1309_delay_callback_t)(uint32_t us);

/**
 *	@brief struct representing ssd1309 display
 */
typedef struct
{
	uint8_t width;			  /** width of display */
	uint8_t height;			  /** height of display */
	uint8_t pages;			  /** stores pages of display (calculated on initialization */
	uint8_t *buffer;		  /** display buffer */
	size_t bufsize;			  /** buffer size */
	ssd1309_spi_callback_t spi_cb; /** SPI callback */
	ssd1309_pin_callback_t pin_cb; /** pin callback */
	ssd1309_delay_callback_t delay;
} ssd1309_t;

enum cursor_type
{
	CURSOR_NONE,
	CURSOR_UNDERSCORE,
	CURSOR_BLOCK
};

typedef struct
{
	uint8_t width;
	uint8_t height;
} vector2_t;

bool ssd1309_init(ssd1309_t *p, uint16_t width, uint16_t height, ssd1309_spi_callback_t spi_cb, ssd1309_pin_callback_t pin_cb, ssd1309_delay_callback_t delay_cb);
void ssd1309_deinit(ssd1309_t *p);

void ssd1309_reset(ssd1309_t *p);
void ssd1309_power(ssd1309_t *p, bool on);
void ssd1309_contrast(ssd1309_t *p, uint8_t val);
void ssd1309_invert(ssd1309_t *p, bool inv);

void ssd1309_show(ssd1309_t *p);
void ssd1309_clear(ssd1309_t *p);

void ssd1309_clear_pixel(ssd1309_t *p, uint32_t x, uint32_t y);
void ssd1309_draw_pixel(ssd1309_t *p, uint32_t x, uint32_t y);
void ssd1309_invert_pixel(ssd1309_t *p, uint32_t x, uint32_t y);
void ssd1309_draw_line(ssd1309_t *p, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void ssd1309_draw_square(ssd1309_t *p, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void ssd1309_draw_empty_square(ssd1309_t *p, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void ssd1309_invert_square(ssd1309_t *p, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

void ssd1309_bmp_show_image_with_offset(ssd1309_t *p, const uint8_t *data, long size, uint32_t x_offset, uint32_t y_offset);
void ssd1309_bmp_show_image(ssd1309_t *p, const uint8_t *data, long size);

uint8_t ssd1309_draw_char_with_font(ssd1309_t *p, uint32_t x, uint32_t y, uint32_t scale, const GFXfont font, char c);
void ssd1309_draw_char(ssd1309_t *p, uint32_t x, uint32_t y, uint32_t scale, char c);
void ssd1309_draw_string_with_font(ssd1309_t *p, uint32_t x, uint32_t y, uint32_t scale, const GFXfont font, const char *s);
void ssd1309_draw_string(ssd1309_t *p, uint32_t x, uint32_t y, uint32_t scale, const char *s);

vector2_t ssd1309_get_string_size_with_font(const GFXfont font, const char *s);
vector2_t ssd1309_get_string_size(const char *s);

void ssd1309_printf(ssd1309_t *p, uint32_t x, uint32_t y, uint32_t scale, const char *fmt, ...);

void ssd1309_cursor(ssd1309_t *p, uint32_t x, uint32_t y, uint32_t scale, enum cursor_type type);

#endif