#ifndef APP_OLED_DISPLAY_H_
#define APP_OLED_DISPLAY_H_

#include <stddef.h>
#include <stdint.h>

class OledDisplay
{
public:
    OledDisplay();

    bool begin();
    bool is_ready() const;
    void clear();
    void flush();
    void draw_pixel(int x, int y, bool on);
    void fill_rect(int x, int y, int w, int h, bool on);
    void draw_rect(int x, int y, int w, int h, bool on);
    void draw_text(int x, int y, const char *text, bool inverted = false);
    void draw_text_right(int right_x, int y, const char *text, bool inverted = false);

private:
    bool send_command(uint8_t command);
    bool send_command2(uint8_t command, uint8_t value);
    bool send_data(const uint8_t *data, size_t length);
    bool write_spi(const uint8_t *data, size_t length, bool data_mode);
    void draw_char(int x, int y, char ch, bool inverted);

    uint8_t buffer_[128U * 64U / 8U];
    bool ready_;
};

#endif
