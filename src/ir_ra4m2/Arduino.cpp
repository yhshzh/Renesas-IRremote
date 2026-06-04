#include "Arduino.h"

#include <algorithm>
#include <ctype.h>

namespace
{
bool g_dwt_started = false;

void start_cycle_counter()
{
    if (g_dwt_started)
    {
        return;
    }

#if (__CORTEX_M >= 3)
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
#endif
    g_dwt_started = true;
}

bsp_io_port_pin_t to_fsp_pin(uint16_t pin)
{
    return static_cast<bsp_io_port_pin_t>(pin);
}
}

HardwareSerial Serial;

int irremote_ra_strcasecmp(const char * lhs, const char * rhs)
{
    if (lhs == rhs)
    {
        return 0;
    }

    if (lhs == NULL)
    {
        return -1;
    }

    if (rhs == NULL)
    {
        return 1;
    }

    while (*lhs && *rhs)
    {
        const int left = tolower(static_cast<unsigned char>(*lhs));
        const int right = tolower(static_cast<unsigned char>(*rhs));

        if (left != right)
        {
            return left - right;
        }

        lhs++;
        rhs++;
    }

    return tolower(static_cast<unsigned char>(*lhs)) -
           tolower(static_cast<unsigned char>(*rhs));
}

void pinMode(uint16_t pin, uint8_t mode)
{
    uint32_t cfg = IOPORT_CFG_PORT_DIRECTION_INPUT;

    if (mode == OUTPUT)
    {
        cfg = IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_PORT_OUTPUT_LOW | IOPORT_CFG_DRIVE_HIGH;
    }
    else if (mode == INPUT_PULLUP)
    {
        cfg = IOPORT_CFG_PORT_DIRECTION_INPUT | IOPORT_CFG_PULLUP_ENABLE;
    }

    (void) R_IOPORT_PinCfg(&g_ioport_ctrl, to_fsp_pin(pin), cfg);
}

void digitalWrite(uint16_t pin, uint8_t value)
{
    (void) R_IOPORT_PinWrite(&g_ioport_ctrl,
                             to_fsp_pin(pin),
                             value ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
}

int digitalRead(uint16_t pin)
{
    bsp_io_level_t value = BSP_IO_LEVEL_LOW;
    (void) R_IOPORT_PinRead(&g_ioport_ctrl, to_fsp_pin(pin), &value);
    return (value == BSP_IO_LEVEL_HIGH) ? HIGH : LOW;
}

uint32_t micros(void)
{
    start_cycle_counter();

#if (__CORTEX_M >= 3)
    const uint32_t cycles_per_usec = std::max(SystemCoreClock / 1000000UL, 1UL);
    return DWT->CYCCNT / cycles_per_usec;
#else
    return 0;
#endif
}

uint32_t millis(void)
{
    return micros() / 1000UL;
}

void delayMicroseconds(uint32_t usec)
{
    R_BSP_SoftwareDelay(usec, BSP_DELAY_UNITS_MICROSECONDS);
}

void delay(uint32_t msec)
{
    R_BSP_SoftwareDelay(msec, BSP_DELAY_UNITS_MILLISECONDS);
}

void yield(void)
{
}
