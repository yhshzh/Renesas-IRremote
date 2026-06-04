#include "hal_data.h"
#include "Arduino.h"
#include "IRac.h"
#include "IRrecv.h"
#include "IRsend.h"
#include "ir_Coolix.h"

#ifndef IRREMOTE_RA4M2_SEND_DEMO
#define IRREMOTE_RA4M2_SEND_DEMO 0
#endif

#ifndef IRREMOTE_RA4M2_RECV_DEMO
#define IRREMOTE_RA4M2_RECV_DEMO 0
#endif

#ifndef IRREMOTE_RA4M2_DEMO_TX_PIN
#define IRREMOTE_RA4M2_DEMO_TX_PIN BSP_IO_PORT_00_PIN_05
#endif

#ifndef IRREMOTE_RA4M2_DEMO_RX_PIN
#define IRREMOTE_RA4M2_DEMO_RX_PIN BSP_IO_PORT_00_PIN_05
#endif

#ifndef IRREMOTE_RA4M2_RECV_BUFFER_SIZE
#define IRREMOTE_RA4M2_RECV_BUFFER_SIZE 1024
#endif

#ifndef IRREMOTE_RA4M2_RECV_TIMEOUT_MS
#define IRREMOTE_RA4M2_RECV_TIMEOUT_MS 50
#endif

#ifndef IRREMOTE_RA4M2_DEBUG_RAW_DUMP_SIZE
#define IRREMOTE_RA4M2_DEBUG_RAW_DUMP_SIZE 256
#endif

volatile decode_type_t g_irremote_last_decode_type = UNKNOWN;
volatile uint64_t g_irremote_last_value = 0;
volatile uint16_t g_irremote_last_bits = 0;
volatile uint16_t g_irremote_last_rawlen = 0;
volatile bool g_irremote_last_overflow = false;
volatile uint32_t g_irremote_decode_count = 0;
volatile uint16_t g_irremote_last_raw_dump_len = 0;
volatile uint16_t g_irremote_last_rawbuf[IRREMOTE_RA4M2_DEBUG_RAW_DUMP_SIZE] = {};

volatile bool g_irremote_ac_state_valid = false;
volatile decode_type_t g_irremote_ac_protocol = UNKNOWN;
volatile int16_t g_irremote_ac_model = -1;
volatile bool g_irremote_ac_power = false;
volatile int16_t g_irremote_ac_mode = -1;
volatile float g_irremote_ac_degrees = 0.0f;
volatile bool g_irremote_ac_celsius = true;
volatile int16_t g_irremote_ac_fanspeed = 0;
volatile int16_t g_irremote_ac_swingv = -1;
volatile int16_t g_irremote_ac_swingh = -1;
volatile bool g_irremote_ac_quiet = false;
volatile bool g_irremote_ac_turbo = false;
volatile bool g_irremote_ac_econo = false;
volatile bool g_irremote_ac_light = false;
volatile bool g_irremote_ac_filter = false;
volatile bool g_irremote_ac_clean = false;
volatile bool g_irremote_ac_beep = false;
volatile int16_t g_irremote_ac_sleep = -1;
volatile int16_t g_irremote_ac_clock = -1;
volatile int16_t g_irremote_ac_command = 0;
volatile bool g_irremote_ac_ifeel = false;
volatile float g_irremote_ac_sensor_temperature = 0.0f;

#if IRREMOTE_RA4M2_RECV_DEMO
namespace
{
stdAc::state_t g_previous_ac_state;
bool g_previous_ac_state_valid = false;

bool is_coolix_family(const decode_type_t protocol)
{
    return (protocol == COOLIX) || (protocol == COOLIX48);
}

const stdAc::state_t *previous_ac_state_for(const decode_type_t protocol)
{
    if (!g_previous_ac_state_valid)
    {
        return NULL;
    }

    if (g_previous_ac_state.protocol == protocol)
    {
        return &g_previous_ac_state;
    }

    if (is_coolix_family(protocol) && is_coolix_family(g_previous_ac_state.protocol))
    {
        return &g_previous_ac_state;
    }

    return NULL;
}

void clear_ac_debug_state(void)
{
    g_irremote_ac_state_valid = false;
    g_irremote_ac_protocol = UNKNOWN;
    g_irremote_ac_model = -1;
    g_irremote_ac_power = false;
    g_irremote_ac_mode = -1;
    g_irremote_ac_degrees = 0.0f;
    g_irremote_ac_celsius = true;
    g_irremote_ac_fanspeed = 0;
    g_irremote_ac_swingv = -1;
    g_irremote_ac_swingh = -1;
    g_irremote_ac_quiet = false;
    g_irremote_ac_turbo = false;
    g_irremote_ac_econo = false;
    g_irremote_ac_light = false;
    g_irremote_ac_filter = false;
    g_irremote_ac_clean = false;
    g_irremote_ac_beep = false;
    g_irremote_ac_sleep = -1;
    g_irremote_ac_clock = -1;
    g_irremote_ac_command = 0;
    g_irremote_ac_ifeel = false;
    g_irremote_ac_sensor_temperature = 0.0f;
}

void publish_ac_debug_state(const stdAc::state_t &state)
{
    g_irremote_ac_protocol = state.protocol;
    g_irremote_ac_model = state.model;
    g_irremote_ac_power = state.power;
    g_irremote_ac_mode = static_cast<int16_t>(state.mode);
    g_irremote_ac_degrees = state.degrees;
    g_irremote_ac_celsius = state.celsius;
    g_irremote_ac_fanspeed = static_cast<int16_t>(state.fanspeed);
    g_irremote_ac_swingv = static_cast<int16_t>(state.swingv);
    g_irremote_ac_swingh = static_cast<int16_t>(state.swingh);
    g_irremote_ac_quiet = state.quiet;
    g_irremote_ac_turbo = state.turbo;
    g_irremote_ac_econo = state.econo;
    g_irremote_ac_light = state.light;
    g_irremote_ac_filter = state.filter;
    g_irremote_ac_clean = state.clean;
    g_irremote_ac_beep = state.beep;
    g_irremote_ac_sleep = state.sleep;
    g_irremote_ac_clock = state.clock;
    g_irremote_ac_command = static_cast<int16_t>(state.command);
    g_irremote_ac_ifeel = state.iFeel;
    g_irremote_ac_sensor_temperature = state.sensorTemperature;
    g_irremote_ac_state_valid = true;
}

void update_ac_debug_state(const decode_results *results)
{
    stdAc::state_t state;
    const stdAc::state_t *previous = previous_ac_state_for(results->decode_type);

    if (IRAcUtils::decodeToState(results, &state, previous))
    {
        g_previous_ac_state = state;
        g_previous_ac_state_valid = true;
        publish_ac_debug_state(state);
    }
#if DECODE_COOLIX48
    else if (results->decode_type == COOLIX48)
    {
        IRCoolixAC ac(kGpioUnused);
        ac.setRawFromCoolix48(results->value);
        state = ac.toCommon(previous);
        state.protocol = COOLIX48;
        g_previous_ac_state = state;
        g_previous_ac_state_valid = true;
        publish_ac_debug_state(state);
    }
#endif
    else
    {
        clear_ac_debug_state();
    }
}
}
#endif

#if (1 == BSP_MULTICORE_PROJECT) && BSP_TZ_SECURE_BUILD
bsp_ipc_semaphore_handle_t g_core_start_semaphore =
{
    .semaphore_num = 0
};
#endif

/*******************************************************************************************************************//**
 * main() is generated by the RA Configuration editor and is used to generate threads if an RTOS is used.  This function
 * is called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void)
{
    /* Wake up 2nd core if this is first core and we are inside a multicore project. */
#if (0 == _RA_CORE) && (1 == BSP_MULTICORE_PROJECT) && !BSP_TZ_NONSECURE_BUILD

#if BSP_TZ_SECURE_BUILD
    /* Take semaphore so 2nd core can clear it */
    R_BSP_IpcSemaphoreTake(&g_core_start_semaphore);
#endif

    R_BSP_SecondaryCoreStart();

#if BSP_TZ_SECURE_BUILD
    /* Wait for 2nd core to start and clear semaphore */
    while(FSP_ERR_IN_USE == R_BSP_IpcSemaphoreTake(&g_core_start_semaphore))
    {
        ;
    }
#endif
#endif

#if (1 == _RA_CORE) && (1 == BSP_MULTICORE_PROJECT) && BSP_TZ_SECURE_BUILD
    /* Signal to 1st core that 2nd core has started */
    R_BSP_IpcSemaphoreGive(&g_core_start_semaphore);
#endif

#if BSP_TZ_SECURE_BUILD
    /* Enter non-secure code */
    R_BSP_NonSecureEnter();
#endif

#if IRREMOTE_RA4M2_SEND_DEMO
    IRsend irsend(IRREMOTE_RA4M2_DEMO_TX_PIN);
    irsend.begin();

    while (true)
    {
        irsend.sendNEC(irsend.encodeNEC(0x00FF, 0x10));
        delay(1000);
    }
#elif IRREMOTE_RA4M2_RECV_DEMO
    IRrecv irrecv(IRREMOTE_RA4M2_DEMO_RX_PIN,
                  IRREMOTE_RA4M2_RECV_BUFFER_SIZE,
                  IRREMOTE_RA4M2_RECV_TIMEOUT_MS);
    decode_results results;
    irrecv.enableIRIn(true);

    while (true)
    {
        if (irrecv.decode(&results))
        {
            g_irremote_last_decode_type = results.decode_type;
            g_irremote_last_value = results.value;
            g_irremote_last_bits = results.bits;
            g_irremote_last_rawlen = results.rawlen;
            g_irremote_last_overflow = results.overflow;
            g_irremote_last_raw_dump_len =
                (results.rawlen < IRREMOTE_RA4M2_DEBUG_RAW_DUMP_SIZE) ?
                results.rawlen : IRREMOTE_RA4M2_DEBUG_RAW_DUMP_SIZE;

            for (uint16_t i = 0; i < g_irremote_last_raw_dump_len; i++)
            {
                g_irremote_last_rawbuf[i] = results.rawbuf[i];
            }

            update_ac_debug_state(&results);
            g_irremote_decode_count++;
            irrecv.resume();
        }
    }
#else
    while (true)
    {
        __WFI();
    }
#endif
}

#if BSP_TZ_SECURE_BUILD

FSP_CPP_HEADER
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ();

/* Trustzone Secure Projects require at least one nonsecure callable function in order to build (Remove this if it is not required to build). */
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ()
{

}
FSP_CPP_FOOTER

#endif
