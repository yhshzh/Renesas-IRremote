#ifndef ARDUINO_H_
#define ARDUINO_H_

#include <stddef.h>
#include <stdint.h>
#include <string>

#include "hal_data.h"

#ifndef F
#define F(x) x
#endif

#ifndef HIGH
#define HIGH 0x1
#endif

#ifndef LOW
#define LOW 0x0
#endif

#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

#define CHANGE 0x3

class String : public std::string
{
public:
    String() : std::string() {}
    String(const char * value) : std::string(value ? value : "") {}
    String(const std::string & value) : std::string(value) {}
    String(char value) : std::string(1U, value) {}

    using std::string::operator=;
    using std::string::operator+=;

    String substring(size_t start) const
    {
        return String(substr(start));
    }

    String substring(size_t start, size_t end) const
    {
        if (end <= start)
        {
            return String();
        }

        return String(substr(start, end - start));
    }
};

class HardwareSerial
{
public:
    void begin(unsigned long baud)
    {
        (void) baud;
    }

    template <typename T>
    size_t print(const T & value)
    {
        (void) value;
        return 0;
    }

    template <typename T>
    size_t println(const T & value)
    {
        (void) value;
        return 0;
    }

    size_t println(void)
    {
        return 0;
    }
};

extern HardwareSerial Serial;

int irremote_ra_strcasecmp(const char * lhs, const char * rhs);

void pinMode(uint16_t pin, uint8_t mode);
void digitalWrite(uint16_t pin, uint8_t value);
int digitalRead(uint16_t pin);
uint32_t micros(void);
uint32_t millis(void);
void delayMicroseconds(uint32_t usec);
void delay(uint32_t msec);
void yield(void);

using byte = uint8_t;

#endif
