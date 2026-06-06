#ifndef IR_RA4M2_GPT_CARRIER_H_
#define IR_RA4M2_GPT_CARRIER_H_

#include <stdint.h>

#ifndef IRREMOTE_RA4M2_USE_GPT_SEND
#define IRREMOTE_RA4M2_USE_GPT_SEND 0
#endif

bool irremote_ra4m2_gpt_carrier_begin(uint16_t pin, uint8_t output_on);
bool irremote_ra4m2_gpt_carrier_configure(uint16_t pin,
                                          uint8_t output_on,
                                          uint32_t frequency_hz,
                                          uint8_t duty_percent);
bool irremote_ra4m2_gpt_carrier_handles_pin(uint16_t pin);
bool irremote_ra4m2_gpt_carrier_ready(void);
bool irremote_ra4m2_gpt_carrier_mark_start(void);
void irremote_ra4m2_gpt_carrier_mark_stop(void);
uint16_t irremote_ra4m2_gpt_carrier_estimate_pulses(uint16_t usec);

#endif
