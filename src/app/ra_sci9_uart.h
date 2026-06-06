#ifndef APP_RA_SCI9_UART_H_
#define APP_RA_SCI9_UART_H_

#include <stddef.h>
#include <stdint.h>

class RaSci9Uart
{
public:
    RaSci9Uart();

    void begin(uint32_t baud);
    bool available() const;
    bool read(uint8_t *value);
    void write(uint8_t value);
    void write(const char *text);
    void write(const uint8_t *data, size_t length);
    void flush_tx();

private:
    uint32_t baud_;
};

#endif
