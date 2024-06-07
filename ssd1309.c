#include "ssd1309.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "fonts/Font5x7FixedMono.h"

#define SSD1309_DEFAULT_FONT Font5x7FixedMono

#define SSD1309_default_memoryAddressingMode 0x00	 /** 0x00 = horizontal, 0x01 = vertical, 0x02 = page */
#define SSD1309_default_contrastControl 0xFF		 /** 0x00 - 0xFF */
#define SSD1309_default_multiplexRatio 0x3F			 /** 0x0F - 0x3F */
#define SSD1309_default_displayOffset 0x00			 /** 0x00 - 0x3F */
#define SSD1309_default_displayClockDivideRatio 0x80 /** 0x00 - 0xFF */
#define SSD1309_default_preChargePeriod 0x22		 /** 0x00 - 0xFF */
#define SSD1309_default_COMpinsHWconfig 0x12		 /** 0x02 - 0x12 */
#define SSD1309_default_VCOMHdeselectLevel 0x40		 /** 0x00 - 0x7F */

#define SSD1309_setContrastControl 0x81			/** set contrast control */
#define SSD1309_followRAMcontent 0xA4			/** resume to RAM content display */
#define SSD1309_allPixelsOn 0xA5				/** all pixels on */
#define SSD1309_inversionOff 0xA6				/** normal display */
#define SSD1309_inversionOn 0xA7				/** inverted display */
#define SSD1309_pwrOff 0xAE						/** power off */
#define SSD1309_pwrOn 0xAF						/** power on */
#define SSD1309_nop 0xE3						/** nop */
#define SSD1309_setCommandLock 0xFD				/** set command lock */
#define SSD1309_contHScrollSetupRight 0x26		/** continuous horizontal scroll setup right */
#define SSD1309_contHScrollSetupLeft 0x27		/** continuous horizontal scroll setup left */
#define SSD1309_contVHScrollSetupRight 0x29		/** continuous vertical and horizontal scroll setup right */
#define SSD1309_contVHScrollSetupLeft 0x2A		/** continuous vertical and horizontal scroll setup left */
#define SSD1309_deactivateScroll 0x2E			/** deactivate scroll */
#define SSD1309_activateScroll 0x2F				/** activate scroll */
#define SSD1309_setVScrollArea 0xA3				/** set vertical scroll area */
#define SSD1309_contentScrollSetupRight 0x2C	/** content scroll setup right */
#define SSD1309_contentScrollSetupLeft 0x2D		/** content scroll setup left */
#define SSD1309_setLowCSAinPAM 0x00				/** set lower column start address in page addressing mode */
#define SSD1309_setHighCSAinPAM 0x10			/** set higher column start address in page addressing mode */
#define SSD1309_setMemoryAddressingMode 0x20	/** set memory addressing mode */
#define SSD1309_setColumnAddress 0x21			/** set column address */
#define SSD1309_setPageAddress 0x22				/** set page address */
#define SSD1309_setPSAinPAM 0xB0				/** set page start address in page addressing mode */
#define SSD1309_setDisplayStartLine 0x40		/** set display start line */
#define SSD1309_setSegmentMapReset 0xA0			/** set segment map reset */
#define SSD1309_setSegmentMapFlipped 0xA1		/** set segment map flipped */
#define SSD1309_setMultiplexRatio 0xA8			/** set multiplex ratio */
#define SSD1309_setCOMoutputNormal 0xC0			/** set COM output normal */
#define SSD1309_setCOMoutputFlipped 0xC8		/** set COM output flipped */
#define SSD1309_setDisplayOffset 0xD3			/** set display offset */
#define SSD1309_setCOMpinsHWconfig 0xDA			/** set COM pins hardware configuration */
#define SSD1309_setGPIO 0xDC					/** set GPIO */
#define SSD1309_setDisplayClockDivideRatio 0xD5 /** set display clock divide ratio */
#define SSD1309_setPreChargePeriod 0xD9			/** set pre-charge period */
#define SSD1309_setVCOMHdeselectLevel 0xDB		/** set VCOMH deselect level */


inline static void _swap(int32_t *a, int32_t *b)
{
    int32_t t = *a;
    *a = *b;
    *b = t;
}

inline static void _ssd1309_write_command(ssd1309_t *p, uint8_t val)
{
    p->pin_cb(SSD1309_PIN_DC, false);
    p->pin_cb(SSD1309_PIN_CS, false);
    p->spi_cb(&val, 1);
    p->pin_cb(SSD1309_PIN_CS, true);
}

inline static void _ssd1309_write_data(ssd1309_t *p, uint8_t *data, size_t size)
{
    p->pin_cb(SSD1309_PIN_DC, true);
    p->pin_cb(SSD1309_PIN_CS, false);
    p->spi_cb(data, size);
    p->pin_cb(SSD1309_PIN_CS, true);
}

/**
 *   @brief initialize ssd1309 display
 *
 *   @param[in,out] p : pointer to instance of ssd1309_t
 *   @param[in] width : width of display
 *   @param[in] height : heigth of display
 *   @param[in] spi_instance : instance of spi connection
 *   @param[in] cs_pin : SPI chip select pin
 *   @param[in] dc_pin : SPI data/command pin
 *   @param[in] reset_pin : SPI reset pin
 *
 *   @return bool.
 *   @retval true for Success
 *   @retval false if initialization failed
 *
 */
bool ssd1309_init(ssd1309_t *p, uint16_t width, uint16_t height, ssd1309_spi_callback_t spi_cb, ssd1309_pin_callback_t pin_cb, ssd1309_delay_callback_t delay_cb)
{
    p->width = width;
    p->height = height;
    p->pages = height / 8;

    p->spi_cb = spi_cb;
    p->pin_cb = pin_cb;
    p->delay = delay_cb;

    p->bufsize = (p->pages) * (p->width);
    if ((p->buffer = (uint8_t *)malloc(p->bufsize + 1)) == NULL)
    {
        p->bufsize = 0;
        return false;
    }

    ++(p->buffer);

    // Commands specific to SSD1309
    uint8_t cmds[] = {
        SSD1309_setLowCSAinPAM,
        SSD1309_setHighCSAinPAM,
        SSD1309_setMemoryAddressingMode,
        SSD1309_default_memoryAddressingMode,

        SSD1309_setContrastControl,
        SSD1309_default_contrastControl,

        SSD1309_inversionOff,

        SSD1309_setMultiplexRatio,
        SSD1309_default_multiplexRatio,

        SSD1309_setDisplayOffset,
        SSD1309_default_displayOffset,

        SSD1309_setDisplayClockDivideRatio,
        SSD1309_default_displayClockDivideRatio,

        SSD1309_setPreChargePeriod,
        SSD1309_default_preChargePeriod,

        SSD1309_setCOMpinsHWconfig,
        SSD1309_default_COMpinsHWconfig,

        SSD1309_setVCOMHdeselectLevel,
        SSD1309_default_VCOMHdeselectLevel,

        SSD1309_followRAMcontent,
    };

    ssd1309_reset(p);
    ssd1309_power(p, false);
    for (size_t i = 0; i < sizeof(cmds); ++i)
    {
        _ssd1309_write_command(p, cmds[i]);
    }
    ssd1309_power(p, true);
    ssd1309_clear(p);
    ssd1309_show(p);

    return true;
}

/**
 *	@brief deinitialize display
 *
 *	@param[in,out] p : instance of display
 *
 */
void ssd1309_deinit(ssd1309_t *p)
{
    free(p->buffer - 1);
}

/**
 * @brief reset display
 *
 * @param[in] p : instance of display
 *
 */
void ssd1309_reset(ssd1309_t *p)
{
    p->pin_cb(SSD1309_PIN_RST, false);
    p->delay(5);
    p->pin_cb(SSD1309_PIN_RST, true);
    p->delay(10000);
}

/**
 *	@brief turn on/off display
 *
 *	@param[in] p : instance of display
 *   @param[in] on : true for on, false for off
 *
 */
void ssd1309_power(ssd1309_t *p, bool on)
{
    if (on)
        _ssd1309_write_command(p, SSD1309_pwrOn);
    else
        _ssd1309_write_command(p, SSD1309_pwrOff);
}

/**
 *	@brief set contrast of display
 *
 *	@param[in] p : instance of display
 *   @param[in] val : contrast value
 *
 */
void ssd1309_contrast(ssd1309_t *p, uint8_t val)
{
    _ssd1309_write_command(p, SSD1309_setContrastControl);
    _ssd1309_write_command(p, val);
}

/**
 *	@brief invert display
 *
 *	@param[in] p : instance of display
 *   @param[in] inv : true for invert, false for normal
 *
 */
void ssd1309_invert(ssd1309_t *p, bool inv)
{
    if (inv)
        _ssd1309_write_command(p, SSD1309_inversionOn);
    else
        _ssd1309_write_command(p, SSD1309_inversionOff);
}

/**
 *	@brief clear display buffer
 *
 *	@param[in,out] p : instance of display
 *
 */
void ssd1309_clear(ssd1309_t *p)
{
    memset(p->buffer, 0, p->bufsize);
}

/**
 * @brief Draw inverted pixel
 *
 * @param[in,out] p : instance of display
 * @param[in] x : x coordinate of pixel
 * @param[in] y : y coordinate of pixel
 *
 */
void ssd1309_clear_pixel(ssd1309_t *p, uint32_t x, uint32_t y)
{
    if (x >= p->width || y >= p->height)
        return;

    x = p->width - x - 1;
    y = p->height - y - 1;

    p->buffer[x + (y / 8) * p->width] &= ~(1 << (y & 7));
}

/**
 * @brief Draw pixel
 *
 * @param[in,out] p : instance of display
 * @param[in] x : x coordinate of pixel
 * @param[in] y : y coordinate of pixel
 *
 */
void ssd1309_draw_pixel(ssd1309_t *p, uint32_t x, uint32_t y)
{
    if (x >= p->width || y >= p->height)
        return;

    x = p->width - x - 1;
    y = p->height - y - 1;

    p->buffer[x + (y / 8) * p->width] |= (1 << (y & 7));
}

/**
 * @brief Invert pixel
 *
 * @param[in,out] p : instance of display
 * @param[in] x : x coordinate of pixel
 * @param[in] y : y coordinate of pixel
 *
 */
void ssd1309_invert_pixel(ssd1309_t *p, uint32_t x, uint32_t y)
{
    if (x >= p->width || y >= p->height)
        return;

    x = p->width - x - 1;
    y = p->height - y - 1;

    p->buffer[x + (y / 8) * p->width] ^= (1 << (y & 7));
}

/**
 * @brief Draw line
 *
 * @param[in,out] p : instance of display
 * @param[in] x1 : x coordinate of first point
 * @param[in] y1 : y coordinate of first point
 * @param[in] x2 : x coordinate of second point
 * @param[in] y2 : y coordinate of second point
 *
 */
void ssd1309_draw_line(ssd1309_t *p, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    if (x1 > x2)
    {
        _swap(&x1, &x2);
        _swap(&y1, &y2);
    }

    if (x1 == x2)
    {
        if (y1 > y2)
            _swap(&y1, &y2);
        for (int32_t i = y1; i <= y2; ++i)
            ssd1309_draw_pixel(p, x1, i);
        return;
    }

    float m = (float)(y2 - y1) / (float)(x2 - x1);

    for (int32_t i = x1; i <= x2; ++i)
    {
        float y = m * (float)(i - x1) + (float)y1;
        ssd1309_draw_pixel(p, i, (uint32_t)y);
    }
}

/**
 * @brief Draw square
 *
 * @param[in,out] p : instance of display
 * @param[in] x : x coordinate of top left corner
 * @param[in] y : y coordinate of top left corner
 * @param[in] width : width of square
 * @param[in] height : height of square
 *
 */
void ssd1309_draw_square(ssd1309_t *p, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    for (uint32_t i = 0; i < width; ++i)
        for (uint32_t j = 0; j < height; ++j)
            ssd1309_draw_pixel(p, x + i, y + j);
}

/**
 * @brief Draw empty square
 *
 * @param[in,out] p : instance of display
 * @param[in] x : x coordinate of top left corner
 * @param[in] y : y coordinate of top left corner
 * @param[in] width : width of square
 * @param[in] height : height of square
 *
 */
void ssd1309_draw_empty_square(ssd1309_t *p, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    ssd1309_draw_line(p, x, y, x + width, y);
    ssd1309_draw_line(p, x, y + height, x + width, y + height);
    ssd1309_draw_line(p, x, y, x, y + height);
    ssd1309_draw_line(p, x + width, y, x + width, y + height);
}

/**
 * @brief Invert square
 *
 * @param[in,out] p : instance of display
 * @param[in] x : x coordinate of top left corner
 * @param[in] y : y coordinate of top left corner
 * @param[in] width : width of square
 * @param[in] height : height of square
 *
 */
void ssd1309_invert_square(ssd1309_t *p, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    for (uint32_t i = 0; i < width; ++i)
        for (uint32_t j = 0; j < height; ++j)
            ssd1309_invert_pixel(p, x + i, y + j);
}

/**
 * @brief Draw char using Adafruit GFX font
 *
 * @param[in,out] p : instance of display
 * @param[in] x : x coordinate of top left corner
 * @param[in] y : y coordinate of top left corner
 * @param[in] scale : scale of char
 * @param[in] font : font to use
 * @param[in] c : char to draw
 * 
 * @return width of char
 *
 */
uint8_t ssd1309_draw_char_with_font(ssd1309_t *p, uint32_t x, uint32_t y, uint32_t scale, const GFXfont font, char c)
{
    if (c < font.first || c > font.last)
        return 0;

    const GFXglyph glyph = font.glyph[(uint8_t)c - font.first];
    const uint8_t *bitmap = font.bitmap + glyph.bitmapOffset;

    for (uint8_t xpos = 0; xpos < glyph.width; xpos++) {
        for (uint8_t ypos = 0; ypos < glyph.height; ypos++) {
            int bitIndex = ypos * glyph.width + xpos;
            if (bitmap[bitIndex / 8] & (1 << (7 - bitIndex % 8))) {
                if (scale == 1)
                {
                    ssd1309_draw_pixel(p, x + xpos + glyph.xOffset, y + ypos + glyph.yOffset);
                }
                else
                {
                    ssd1309_draw_square(p, x + (xpos + glyph.xOffset) * scale, y + (ypos + glyph.yOffset) * scale, scale, scale);
                }
            }
        }
    }

    return glyph.xAdvance;
}

/**
 * @brief Draw string using Adafruit GFX font
 *
 * @param[in,out] p : instance of display
 * @param[in] x : x coordinate of top left corner
 * @param[in] y : y coordinate of top left corner
 * @param[in] scale : scale of char
 * @param[in] font : font to use
 * @param[in] s : string to draw
 *
 */
void ssd1309_draw_string_with_font(ssd1309_t *p, uint32_t x, uint32_t y, uint32_t scale, const GFXfont font, const char *s)
{
    uint8_t x_n = x;
    for (; *s; s++)
    {
        x_n += ssd1309_draw_char_with_font(p, x_n, y, scale, font, *s) * scale;
    }
}

/**
 * @brief Draw char using default font
 *
 * @param[in,out] p : instance of display
 * @param[in] x : x coordinate of top left corner
 * @param[in] y : y coordinate of top left corner
 * @param[in] scale : scale of char
 * @param[in] c : char to draw
 *
 */
void ssd1309_draw_char(ssd1309_t *p, uint32_t x, uint32_t y, uint32_t scale, char c)
{
    ssd1309_draw_char_with_font(p, x, y, scale, SSD1309_DEFAULT_FONT, c);
}

/**
 * @brief Draw string using default font
 *
 * @param[in,out] p : instance of display
 * @param[in] x : x coordinate of top left corner
 * @param[in] y : y coordinate of top left corner
 * @param[in] scale : scale of char
 * @param[in] s : string to draw
 *
 */
void ssd1309_draw_string(ssd1309_t *p, uint32_t x, uint32_t y, uint32_t scale, const char *s)
{
    ssd1309_draw_string_with_font(p, x, y, scale, SSD1309_DEFAULT_FONT, s);
}

vector2_t ssd1309_get_string_size_with_font(const GFXfont font, const char *s)
{
    vector2_t size = {0, 0};
    for (; *s; s++)
    {
        const GFXglyph glyph = font.glyph[(uint8_t)*s];
        size.width += glyph.xAdvance;
        size.height = font.yAdvance;
    }
    return size;
}

vector2_t ssd1309_get_string_size(const char *s)
{
    return ssd1309_get_string_size_with_font(SSD1309_DEFAULT_FONT, s);
}

/**
 * @brief Draw formatted string using default font
 *
 * @param[in,out] p : instance of display
 * @param[in] x : x coordinate of top left corner
 * @param[in] y : y coordinate of top left corner
 * @param[in] scale : scale of char
 * @param[in] format : format string
 * @param[in] ... : arguments
 *
 */
void ssd1309_printf(ssd1309_t *disp, uint32_t x, uint32_t y, uint32_t scale, const char *format, ...)
{
    char buf[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    uint32_t y_offset = y * SSD1309_DEFAULT_FONT.yAdvance * scale;
    uint32_t x_offset = x * 6 * scale;

    ssd1309_draw_string(disp, x_offset, y_offset, scale, buf);
}

void _ssd1309_get_char_position_size(uint32_t *x, uint32_t *y, uint32_t *width, uint32_t *height, uint32_t scale)
{
    *x = 6 * scale * (*x);
    *y = 8 * scale * (*y);
    *width = 6 * scale;
    *height = 8 * scale;
}

void ssd1309_cursor(ssd1309_t *disp, uint32_t x, uint32_t y, uint32_t scale, enum cursor_type type)
{
    uint32_t width = 0;
    uint32_t height = 0;
    _ssd1309_get_char_position_size(&x, &y, &width, &height, scale);

    switch (type)
    {
    case CURSOR_NONE:
        break;
    case CURSOR_UNDERSCORE:
        ssd1309_draw_line(disp, x, y + height - 1, x + width - 2, y + height - 1);
        break;
    case CURSOR_BLOCK:
        ssd1309_invert_square(disp, x - 1, y - 1, width + 1, height + 1);
        break;
    default:
        break;
    }
}

static inline uint32_t _ssd1309_bmp_get_val(const uint8_t *data, const size_t offset, uint8_t size)
{
    switch (size)
    {
    case 1:
        return data[offset];
    case 2:
        return data[offset] | (data[offset + 1] << 8);
    case 4:
        return data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24);
    default:
        return 0;
    }
    return 0;
}

void ssd1309_bmp_show_image_with_offset(ssd1309_t *p, const uint8_t *data, const long size, uint32_t x_offset,
                                        uint32_t y_offset)
{
    if (size < 54) // data smaller than header
        return;

    const uint32_t bf_off_bits = _ssd1309_bmp_get_val(data, 10, 4);
    const uint32_t bi_size = _ssd1309_bmp_get_val(data, 14, 4);
    const int32_t bi_width = (int32_t)_ssd1309_bmp_get_val(data, 18, 4);
    const int32_t bi_height = (int32_t)_ssd1309_bmp_get_val(data, 22, 4);
    const uint16_t bi_bit_count = (uint16_t)_ssd1309_bmp_get_val(data, 28, 2);
    const uint32_t bi_compression = _ssd1309_bmp_get_val(data, 30, 4);

    if (bi_bit_count != 1) // image not monochrome
        return;

    if (bi_compression != 0) // image compressed
        return;

    const int table_start = 14 + bi_size;
    uint8_t color_val = 0;

    for (uint8_t i = 0; i < 2; ++i)
    {
        if (!((data[table_start + i * 4] << 16) | (data[table_start + i * 4 + 1] << 8) | data[table_start + i * 4 + 2]))
        {
            color_val = i;
            break;
        }
    }

    uint32_t bytes_per_line = (bi_width / 8) + (bi_width & 7 ? 1 : 0);
    if (bytes_per_line & 3)
        bytes_per_line = (bytes_per_line ^ (bytes_per_line & 3)) + 4;

    const uint8_t *img_data = data + bf_off_bits;

    int step = bi_height > 0 ? -1 : 1;
    int border = bi_height > 0 ? -1 : bi_height;
    for (uint32_t y = bi_height > 0 ? bi_height - 1 : 0; y != border; y += step)
    {
        for (uint32_t x = 0; x < bi_width; ++x)
        {
            if (((img_data[x >> 3] >> (7 - (x & 7))) & 1) == color_val)
                ssd1309_draw_pixel(p, x_offset + x, y_offset + y);
        }
        img_data += bytes_per_line;
    }
}

void ssd1309_bmp_show_image(ssd1309_t *p, const uint8_t *data, const long size)
{
    ssd1309_bmp_show_image_with_offset(p, data, size, 0, 0);
}

void ssd1309_show(ssd1309_t *p)
{
    _ssd1309_write_command(p, SSD1309_setColumnAddress);
    _ssd1309_write_command(p, 0);            // Column start address (0 = reset)
    _ssd1309_write_command(p, p->width - 1); // Column end address (127 = reset)

    _ssd1309_write_command(p, SSD1309_setPageAddress);
    _ssd1309_write_command(p, 0);            // Page start address (0 = reset)
    _ssd1309_write_command(p, p->pages - 1); // Page end address

    _ssd1309_write_data(p, p->buffer, p->bufsize);
}