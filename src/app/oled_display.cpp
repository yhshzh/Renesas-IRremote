#include "app/oled_display.h"

#include <ctype.h>
#include <string.h>

#include "Arduino.h"
#include "app/app_board_config.h"
#include "hal_data.h"

namespace
{
volatile bool g_spi_done = false;
volatile bool g_spi_error = false;

const uint8_t *glyph_for(char ch)
{
    static const uint8_t blank[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    static const uint8_t unknown[5] = {0x3E, 0x41, 0x5D, 0x41, 0x3E};
    static const uint8_t colon[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
    static const uint8_t dot[5] = {0x00, 0x60, 0x60, 0x00, 0x00};
    static const uint8_t dash[5] = {0x08, 0x08, 0x08, 0x08, 0x08};
    static const uint8_t slash[5] = {0x20, 0x10, 0x08, 0x04, 0x02};
    static const uint8_t percent[5] = {0x62, 0x64, 0x08, 0x13, 0x23};
    static const uint8_t gt[5] = {0x00, 0x41, 0x22, 0x14, 0x08};

    static const uint8_t digits[10][5] = {
        {0x3E, 0x51, 0x49, 0x45, 0x3E},
        {0x00, 0x42, 0x7F, 0x40, 0x00},
        {0x42, 0x61, 0x51, 0x49, 0x46},
        {0x21, 0x41, 0x45, 0x4B, 0x31},
        {0x18, 0x14, 0x12, 0x7F, 0x10},
        {0x27, 0x45, 0x45, 0x45, 0x39},
        {0x3C, 0x4A, 0x49, 0x49, 0x30},
        {0x01, 0x71, 0x09, 0x05, 0x03},
        {0x36, 0x49, 0x49, 0x49, 0x36},
        {0x06, 0x49, 0x49, 0x29, 0x1E},
    };

    static const uint8_t letters[26][5] = {
        {0x7E, 0x11, 0x11, 0x11, 0x7E},
        {0x7F, 0x49, 0x49, 0x49, 0x36},
        {0x3E, 0x41, 0x41, 0x41, 0x22},
        {0x7F, 0x41, 0x41, 0x22, 0x1C},
        {0x7F, 0x49, 0x49, 0x49, 0x41},
        {0x7F, 0x09, 0x09, 0x09, 0x01},
        {0x3E, 0x41, 0x49, 0x49, 0x7A},
        {0x7F, 0x08, 0x08, 0x08, 0x7F},
        {0x00, 0x41, 0x7F, 0x41, 0x00},
        {0x20, 0x40, 0x41, 0x3F, 0x01},
        {0x7F, 0x08, 0x14, 0x22, 0x41},
        {0x7F, 0x40, 0x40, 0x40, 0x40},
        {0x7F, 0x02, 0x0C, 0x02, 0x7F},
        {0x7F, 0x04, 0x08, 0x10, 0x7F},
        {0x3E, 0x41, 0x41, 0x41, 0x3E},
        {0x7F, 0x09, 0x09, 0x09, 0x06},
        {0x3E, 0x41, 0x51, 0x21, 0x5E},
        {0x7F, 0x09, 0x19, 0x29, 0x46},
        {0x46, 0x49, 0x49, 0x49, 0x31},
        {0x01, 0x01, 0x7F, 0x01, 0x01},
        {0x3F, 0x40, 0x40, 0x40, 0x3F},
        {0x1F, 0x20, 0x40, 0x20, 0x1F},
        {0x3F, 0x40, 0x38, 0x40, 0x3F},
        {0x63, 0x14, 0x08, 0x14, 0x63},
        {0x07, 0x08, 0x70, 0x08, 0x07},
        {0x61, 0x51, 0x49, 0x45, 0x43},
    };

    if (' ' == ch)
    {
        return blank;
    }
    if ((ch >= '0') && (ch <= '9'))
    {
        return digits[ch - '0'];
    }

    ch = static_cast<char>(toupper(static_cast<unsigned char>(ch)));
    if ((ch >= 'A') && (ch <= 'Z'))
    {
        return letters[ch - 'A'];
    }

    switch (ch)
    {
        case ':':
            return colon;
        case '.':
            return dot;
        case '-':
        case '_':
            return dash;
        case '/':
            return slash;
        case '%':
            return percent;
        case '>':
            return gt;
        default:
            return unknown;
    }
}

void pin_write(bsp_io_port_pin_t pin, bsp_io_level_t level)
{
    (void) g_ioport.p_api->pinWrite(g_ioport.p_ctrl, pin, level);
}
}

extern "C" void spi_callback(spi_callback_args_t *p_args)
{
    if (NULL == p_args)
    {
        return;
    }

    if (SPI_EVENT_TRANSFER_COMPLETE == p_args->event)
    {
        g_spi_done = true;
    }
    else
    {
        g_spi_error = true;
        g_spi_done = true;
    }
}

OledDisplay::OledDisplay() :
    buffer_(),
    ready_(false)
{
}

bool OledDisplay::begin()
{
    if (ready_)
    {
        return true;
    }

    pin_write(APP_OLED_RESET_PIN, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(200U, BSP_DELAY_UNITS_MILLISECONDS);
    pin_write(APP_OLED_RESET_PIN, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(20U, BSP_DELAY_UNITS_MILLISECONDS);

    fsp_err_t err = APP_OLED_SPI_INSTANCE.p_api->open(APP_OLED_SPI_INSTANCE.p_ctrl,
                                                       APP_OLED_SPI_INSTANCE.p_cfg);
    if ((FSP_SUCCESS != err) && (FSP_ERR_ALREADY_OPEN != err))
    {
        return false;
    }

    ready_ = true;
    send_command(0xAE);
    send_command(0x00);
    send_command(0x10);
    send_command(0x40);
    send_command2(0x81, 0xCF);
    send_command(0xA1);
    send_command(0xC8);
    send_command(0xA6);
    send_command2(0xA8, 0x3F);
    send_command2(0xD3, 0x00);
    send_command2(0xD5, 0x80);
    send_command2(0xD9, 0xF1);
    send_command2(0xDA, 0x12);
    send_command2(0xDB, 0x30);
    send_command2(0x20, 0x02);
    send_command2(0x8D, 0x14);
    clear();
    flush();
    send_command(0xAF);
    return true;
}

bool OledDisplay::is_ready() const
{
    return ready_;
}

void OledDisplay::clear()
{
    memset(buffer_, 0, sizeof(buffer_));
}

void OledDisplay::flush()
{
    if (!ready_)
    {
        return;
    }

    for (uint8_t page = 0U; page < 8U; page++)
    {
        send_command(static_cast<uint8_t>(0xB0U | page));
        send_command(0x00);
        send_command(0x10);
        send_data(&buffer_[static_cast<size_t>(page) * 128U], 128U);
    }
}

void OledDisplay::draw_pixel(int x, int y, bool on)
{
    if ((x < 0) || (x >= APP_OLED_WIDTH) || (y < 0) || (y >= APP_OLED_HEIGHT))
    {
        return;
    }

    const size_t index = static_cast<size_t>(x) + ((static_cast<size_t>(y) >> 3U) * APP_OLED_WIDTH);
    const uint8_t mask = static_cast<uint8_t>(1U << (static_cast<uint8_t>(y) & 0x07U));
    if (on)
    {
        buffer_[index] |= mask;
    }
    else
    {
        buffer_[index] &= static_cast<uint8_t>(~mask);
    }
}

void OledDisplay::fill_rect(int x, int y, int w, int h, bool on)
{
    for (int yy = y; yy < (y + h); yy++)
    {
        for (int xx = x; xx < (x + w); xx++)
        {
            draw_pixel(xx, yy, on);
        }
    }
}

void OledDisplay::draw_rect(int x, int y, int w, int h, bool on)
{
    for (int xx = x; xx < (x + w); xx++)
    {
        draw_pixel(xx, y, on);
        draw_pixel(xx, y + h - 1, on);
    }
    for (int yy = y; yy < (y + h); yy++)
    {
        draw_pixel(x, yy, on);
        draw_pixel(x + w - 1, yy, on);
    }
}

void OledDisplay::draw_text(int x, int y, const char *text, bool inverted)
{
    if (NULL == text)
    {
        return;
    }

    int cursor = x;
    while ('\0' != *text)
    {
        draw_char(cursor, y, *text, inverted);
        cursor += 6;
        text++;
    }
}

void OledDisplay::draw_text_right(int right_x, int y, const char *text, bool inverted)
{
    if (NULL == text)
    {
        return;
    }

    const int width = static_cast<int>(strlen(text)) * 6;
    draw_text(right_x - width, y, text, inverted);
}

bool OledDisplay::send_command(uint8_t command)
{
    return write_spi(&command, 1U, false);
}

bool OledDisplay::send_command2(uint8_t command, uint8_t value)
{
    return send_command(command) && send_command(value);
}

bool OledDisplay::send_data(const uint8_t *data, size_t length)
{
    return write_spi(data, length, true);
}

bool OledDisplay::write_spi(const uint8_t *data, size_t length, bool data_mode)
{
    if ((NULL == data) || (0U == length))
    {
        return false;
    }

    pin_write(APP_OLED_DC_PIN, data_mode ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
    g_spi_done = false;
    g_spi_error = false;
    const fsp_err_t err = APP_OLED_SPI_INSTANCE.p_api->write(APP_OLED_SPI_INSTANCE.p_ctrl,
                                                             data,
                                                             static_cast<uint32_t>(length),
                                                             SPI_BIT_WIDTH_8_BITS);
    if (FSP_SUCCESS != err)
    {
        return false;
    }

    const uint32_t start = millis();
    while (!g_spi_done)
    {
        if ((millis() - start) > 50U)
        {
            return false;
        }
    }

    return !g_spi_error;
}

void OledDisplay::draw_char(int x, int y, char ch, bool inverted)
{
    const uint8_t *glyph = glyph_for(ch);
    if (inverted)
    {
        fill_rect(x, y, 6, 8, true);
    }

    for (int col = 0; col < 5; col++)
    {
        uint8_t bits = glyph[col];
        for (int row = 0; row < 7; row++)
        {
            const bool pixel = (0U != (bits & (1U << row)));
            draw_pixel(x + col, y + row, inverted ? !pixel : pixel);
        }
    }
}
