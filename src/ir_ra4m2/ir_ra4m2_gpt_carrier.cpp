#include "ir_ra4m2_gpt_carrier.h"

#include <algorithm>

#include "Arduino.h"

#if IRREMOTE_RA4M2_USE_GPT_SEND
#include "r_gpt.h"

#ifndef IRREMOTE_RA4M2_GPT_SEND_INSTANCE
#define IRREMOTE_RA4M2_GPT_SEND_INSTANCE g_ir_tx_gpt
#endif

#ifndef IRREMOTE_RA4M2_GPT_SEND_IO_PIN
#define IRREMOTE_RA4M2_GPT_SEND_IO_PIN GPT_IO_PIN_GTIOCA
#endif

extern const timer_instance_t IRREMOTE_RA4M2_GPT_SEND_INSTANCE;
#endif

namespace
{
#if IRREMOTE_RA4M2_USE_GPT_SEND
bool g_open = false;
bool g_ready = false;
uint16_t g_pin = 0U;
uint8_t g_output_on = HIGH;
uint32_t g_frequency_hz = 0U;

const timer_instance_t *carrier_timer(void)
{
    return &IRREMOTE_RA4M2_GPT_SEND_INSTANCE;
}

bool fsp_ok(fsp_err_t err)
{
    return (FSP_SUCCESS == err) || (FSP_ERR_ALREADY_OPEN == err);
}
#endif
}

bool irremote_ra4m2_gpt_carrier_begin(uint16_t pin, uint8_t output_on)
{
#if IRREMOTE_RA4M2_USE_GPT_SEND
    g_pin = pin;
    g_output_on = output_on;
    g_ready = false;

    if (HIGH != g_output_on)
    {
        return false;
    }

    const timer_instance_t *timer = carrier_timer();
    if (!g_open)
    {
        if (!fsp_ok(timer->p_api->open(timer->p_ctrl, timer->p_cfg)))
        {
            return false;
        }

        g_open = true;
    }

    (void) timer->p_api->stop(timer->p_ctrl);
    (void) timer->p_api->reset(timer->p_ctrl);
    return true;
#else
    (void) pin;
    (void) output_on;
    return false;
#endif
}

bool irremote_ra4m2_gpt_carrier_configure(uint16_t pin,
                                          uint8_t output_on,
                                          uint32_t frequency_hz,
                                          uint8_t duty_percent)
{
#if IRREMOTE_RA4M2_USE_GPT_SEND
    if (0U == frequency_hz)
    {
        frequency_hz = 1U;
    }

    if (!irremote_ra4m2_gpt_carrier_begin(pin, output_on))
    {
        return false;
    }

    timer_info_t info = {};
    const timer_instance_t *timer = carrier_timer();
    if (FSP_SUCCESS != timer->p_api->infoGet(timer->p_ctrl, &info))
    {
        return false;
    }

    uint32_t period_counts = (info.clock_frequency + (frequency_hz / 2U)) / frequency_hz;
    period_counts = std::max<uint32_t>(period_counts, 1U);

    duty_percent = std::min<uint8_t>(duty_percent, 100U);
    uint32_t duty_counts = (period_counts * duty_percent) / 100U;
    if ((duty_percent > 0U) && (0U == duty_counts))
    {
        duty_counts = 1U;
    }
    if ((period_counts > 1U) && (duty_counts >= period_counts))
    {
        duty_counts = period_counts - 1U;
    }

    if (FSP_SUCCESS != timer->p_api->stop(timer->p_ctrl))
    {
        return false;
    }
    if (FSP_SUCCESS != timer->p_api->periodSet(timer->p_ctrl, period_counts))
    {
        return false;
    }
    if (FSP_SUCCESS != timer->p_api->dutyCycleSet(timer->p_ctrl,
                                                  duty_counts,
                                                  IRREMOTE_RA4M2_GPT_SEND_IO_PIN))
    {
        return false;
    }
    if (FSP_SUCCESS != timer->p_api->reset(timer->p_ctrl))
    {
        return false;
    }

    g_ready = true;
    g_frequency_hz = frequency_hz;
    return true;
#else
    (void) pin;
    (void) output_on;
    (void) frequency_hz;
    (void) duty_percent;
    return false;
#endif
}

bool irremote_ra4m2_gpt_carrier_handles_pin(uint16_t pin)
{
#if IRREMOTE_RA4M2_USE_GPT_SEND
    return g_open && (pin == g_pin);
#else
    (void) pin;
    return false;
#endif
}

bool irremote_ra4m2_gpt_carrier_ready(void)
{
#if IRREMOTE_RA4M2_USE_GPT_SEND
    return g_ready;
#else
    return false;
#endif
}

bool irremote_ra4m2_gpt_carrier_mark_start(void)
{
#if IRREMOTE_RA4M2_USE_GPT_SEND
    if (!g_ready)
    {
        return false;
    }

    const timer_instance_t *timer = carrier_timer();
    if (FSP_SUCCESS != timer->p_api->reset(timer->p_ctrl))
    {
        return false;
    }

    return FSP_SUCCESS == timer->p_api->start(timer->p_ctrl);
#else
    return false;
#endif
}

void irremote_ra4m2_gpt_carrier_mark_stop(void)
{
#if IRREMOTE_RA4M2_USE_GPT_SEND
    if (g_open)
    {
        (void) carrier_timer()->p_api->stop(carrier_timer()->p_ctrl);
    }
#endif
}

uint16_t irremote_ra4m2_gpt_carrier_estimate_pulses(uint16_t usec)
{
#if IRREMOTE_RA4M2_USE_GPT_SEND
    if ((0U == g_frequency_hz) || (0U == usec))
    {
        return 0U;
    }

    const uint64_t pulses =
        ((static_cast<uint64_t>(usec) * static_cast<uint64_t>(g_frequency_hz)) + 999999ULL) / 1000000ULL;
    return static_cast<uint16_t>(std::min<uint64_t>(pulses, UINT16_MAX));
#else
    (void) usec;
    return 0U;
#endif
}
