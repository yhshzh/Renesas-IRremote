#include "app/ra_sci9_uart.h"

#include "hal_data.h"

#include <string.h>

namespace
{
constexpr uint16_t kRxBufferSize = 2048U;
constexpr uint16_t kRxBufferMask = kRxBufferSize - 1U;

static_assert((kRxBufferSize & kRxBufferMask) == 0U, "UART RX buffer size must be a power of two");

uint8_t g_rx_buffer[kRxBufferSize];
volatile uint16_t g_rx_head = 0U;
volatile uint16_t g_rx_tail = 0U;
volatile uint32_t g_rx_overflow_count = 0U;
volatile uint32_t g_uart_error_count = 0U;
volatile bool g_uart_open = false;
volatile bool g_tx_complete = true;

uint16_t next_rx_index(uint16_t index)
{
    return static_cast<uint16_t>((index + 1U) & kRxBufferMask);
}

void rx_push(uint8_t value)
{
    const uint16_t head = g_rx_head;
    const uint16_t next = next_rx_index(head);
    if (next == g_rx_tail)
    {
        g_rx_tail = next_rx_index(g_rx_tail);
        g_rx_overflow_count++;
    }

    g_rx_buffer[head] = value;
    g_rx_head = next;
}

bool rx_pop(uint8_t *value)
{
    if (NULL == value)
    {
        return false;
    }

    const uint16_t tail = g_rx_tail;
    if (tail == g_rx_head)
    {
        return false;
    }

    *value = g_rx_buffer[tail];
    g_rx_tail = next_rx_index(tail);
    return true;
}

bool open_uart()
{
    if (g_uart_open)
    {
        return true;
    }

    g_rx_head = 0U;
    g_rx_tail = 0U;
    g_rx_overflow_count = 0U;
    g_uart_error_count = 0U;
    g_tx_complete = true;

    const fsp_err_t err = g_esp_uart.p_api->open(g_esp_uart.p_ctrl, g_esp_uart.p_cfg);
    if ((FSP_SUCCESS == err) || (FSP_ERR_ALREADY_OPEN == err))
    {
        g_uart_open = true;
    }

    return g_uart_open;
}

void wait_tx_complete()
{
    while (!g_tx_complete)
    {
        ;
    }
}
}

extern "C" void esp_uart_callback(uart_callback_args_t *p_args)
{
    if (NULL == p_args)
    {
        return;
    }

    switch (p_args->event)
    {
        case UART_EVENT_RX_CHAR:
        {
            rx_push(static_cast<uint8_t>(p_args->data));
            break;
        }

        case UART_EVENT_TX_COMPLETE:
        {
            g_tx_complete = true;
            break;
        }

        case UART_EVENT_ERR_PARITY:
        case UART_EVENT_ERR_FRAMING:
        case UART_EVENT_ERR_OVERFLOW:
        case UART_EVENT_BREAK_DETECT:
        {
            g_uart_error_count++;
            break;
        }

        default:
        {
            break;
        }
    }
}

RaSci9Uart::RaSci9Uart() :
    baud_(0U)
{
}

void RaSci9Uart::begin(uint32_t baud)
{
    baud_ = baud;
    (void) baud_;
    (void) open_uart();
}

bool RaSci9Uart::available() const
{
    return g_rx_head != g_rx_tail;
}

bool RaSci9Uart::read(uint8_t *value)
{
    return rx_pop(value);
}

void RaSci9Uart::write(uint8_t value)
{
    write(&value, 1U);
}

void RaSci9Uart::write(const char *text)
{
    if (NULL == text)
    {
        return;
    }

    write(reinterpret_cast<const uint8_t *>(text), strlen(text));
}

void RaSci9Uart::write(const uint8_t *data, size_t length)
{
    if ((NULL == data) || (0U == length) || !open_uart())
    {
        return;
    }

    wait_tx_complete();
    g_tx_complete = false;
    const fsp_err_t err = g_esp_uart.p_api->write(g_esp_uart.p_ctrl,
                                                  data,
                                                  static_cast<uint32_t>(length));
    if (FSP_SUCCESS != err)
    {
        g_tx_complete = true;
        return;
    }

    wait_tx_complete();
}

void RaSci9Uart::flush_tx()
{
    wait_tx_complete();
}
