// RA4M2 receive backend for IRremoteESP8266.
// Common match/decode helpers are adapted from IRremoteESP8266/src/IRrecv.cpp.

#include "IRrecv.h"

#include <algorithm>
#include <stddef.h>
#include <stdint.h>

#include "Arduino.h"
#include "IRremoteESP8266.h"
#include "IRutils.h"

#ifndef IRREMOTE_RA4M2_RECV_ACTIVE_LEVEL
#define IRREMOTE_RA4M2_RECV_ACTIVE_LEVEL LOW
#endif

#ifndef IRREMOTE_RA4M2_RECV_START_TIMEOUT_US
#define IRREMOTE_RA4M2_RECV_START_TIMEOUT_US 50000UL
#endif

#ifndef IRREMOTE_RA4M2_USE_SYSTICK_RECV
#define IRREMOTE_RA4M2_USE_SYSTICK_RECV 1
#endif

#ifndef IRREMOTE_RA4M2_RECV_SYSTICK_US
#define IRREMOTE_RA4M2_RECV_SYSTICK_US 50UL
#endif

#if IRREMOTE_RA4M2_USE_SYSTICK_RECV
extern "C" void SysTick_Handler(void);
#endif

namespace
{
atomic_irparams_t g_params;
uint16_t g_recvpin = 0;

#if IRREMOTE_RA4M2_USE_SYSTICK_RECV
volatile bool g_systick_running = false;
volatile bool g_capture_enabled = false;
volatile int g_last_level = HIGH;
volatile uint32_t g_last_edge_us = 0;
#endif

uint32_t elapsed_since(uint32_t start, uint32_t now)
{
    return (start <= now) ? (now - start) : (UINT32_MAX - start + 1UL + now);
}

uint16_t usecs_to_ticks(uint32_t usecs)
{
    const uint32_t ticks = usecs / kRawTick;
    return static_cast<uint16_t>(std::min(ticks, static_cast<uint32_t>(UINT16_MAX)));
}

#if IRREMOTE_RA4M2_USE_SYSTICK_RECV
bool start_systick()
{
    const uint64_t cycles = (static_cast<uint64_t>(SystemCoreClock) *
                             static_cast<uint64_t>(IRREMOTE_RA4M2_RECV_SYSTICK_US)) /
                            1000000ULL;
    const uint32_t ticks = static_cast<uint32_t>(std::max<uint64_t>(cycles, 1ULL));

    if ((ticks - 1UL) > SysTick_LOAD_RELOAD_Msk)
    {
        g_systick_running = false;
        return false;
    }

    g_systick_running = (SysTick_Config(ticks) == 0UL);
    return g_systick_running;
}

void stop_systick()
{
    SysTick->CTRL = 0UL;
    SysTick->LOAD = 0UL;
    SysTick->VAL = 0UL;
    g_systick_running = false;
}

void capture_systick_sample()
{
    if (!g_capture_enabled || (g_params.rawbuf == NULL) ||
        (g_params.rcvstate == kStopState))
    {
        return;
    }

    const int active_level = IRREMOTE_RA4M2_RECV_ACTIVE_LEVEL;
    const int current_level = digitalRead(g_recvpin);
    const uint32_t now = micros();

    if (g_params.rcvstate == kIdleState)
    {
        if (current_level == active_level)
        {
            g_params.rawlen = 0;
            g_params.overflow = false;
            g_params.rawbuf[g_params.rawlen++] = 1;
            g_last_level = current_level;
            g_last_edge_us = now;
            g_params.rcvstate = kMarkState;
        }

        return;
    }

    const uint32_t elapsed = elapsed_since(g_last_edge_us, now);

    if (current_level != g_last_level)
    {
        if (g_params.rawlen >= g_params.bufsize)
        {
            g_params.overflow = true;
            g_params.rcvstate = kStopState;
            return;
        }

        g_params.rawbuf[g_params.rawlen++] = usecs_to_ticks(elapsed);
        g_last_level = current_level;
        g_last_edge_us = now;
        g_params.rcvstate = (current_level == active_level) ? kMarkState : kSpaceState;
    }
    else if ((current_level != active_level) &&
             (elapsed >= MS_TO_USEC(g_params.timeout)) &&
             (g_params.rawlen > 1U))
    {
        g_params.rcvstate = kStopState;
    }
    else if ((current_level == active_level) &&
             (elapsed >= MS_TO_USEC(g_params.timeout)))
    {
        g_params.overflow = true;
        g_params.rcvstate = kStopState;
    }
}
#endif

bool capture_polling()
{
    const int active_level = IRREMOTE_RA4M2_RECV_ACTIVE_LEVEL;
    const uint32_t start_wait = micros();

    while (digitalRead(g_recvpin) != active_level)
    {
        if (IRREMOTE_RA4M2_RECV_START_TIMEOUT_US == 0UL)
        {
            return false;
        }

        if (elapsed_since(start_wait, micros()) >= IRREMOTE_RA4M2_RECV_START_TIMEOUT_US)
        {
            return false;
        }
    }

    g_params.rawlen = 0;
    g_params.overflow = false;
    g_params.rcvstate = kMarkState;
    g_params.rawbuf[g_params.rawlen++] = 1;

    int last_level = active_level;
    uint32_t last_edge = micros();
    const uint32_t timeout_usecs = MS_TO_USEC(g_params.timeout);

    while (g_params.rawlen < g_params.bufsize)
    {
        const int current_level = digitalRead(g_recvpin);
        const uint32_t now = micros();
        const uint32_t elapsed = elapsed_since(last_edge, now);

        if (current_level != last_level)
        {
            g_params.rawbuf[g_params.rawlen++] = usecs_to_ticks(elapsed);
            last_level = current_level;
            last_edge = now;
            g_params.rcvstate = (current_level == active_level) ? kMarkState : kSpaceState;
        }
        else if ((current_level != active_level) && (elapsed >= timeout_usecs))
        {
            g_params.rcvstate = kStopState;
            return g_params.rawlen > 1;
        }
        else if ((current_level == active_level) && (elapsed >= timeout_usecs))
        {
            g_params.overflow = true;
            g_params.rcvstate = kStopState;
            return false;
        }
    }

    g_params.overflow = true;
    g_params.rcvstate = kStopState;
    return g_params.rawlen > 1;
}
}

#if IRREMOTE_RA4M2_USE_SYSTICK_RECV
extern "C" void SysTick_Handler(void)
{
    capture_systick_sample();
}
#endif

IRrecv::IRrecv(const uint16_t recvpin, const uint16_t bufsize,
               const uint8_t timeout, const bool save_buffer)
{
    g_recvpin = recvpin;
    g_params.recvpin = static_cast<uint8_t>(recvpin & 0xFFU);
    g_params.bufsize = bufsize;
    g_params.timeout = std::min(timeout, static_cast<uint8_t>(kMaxTimeoutMs));
    g_params.rawbuf = new uint16_t[bufsize];
    g_params.rawlen = 0;
    g_params.overflow = false;
    g_params.rcvstate = kIdleState;
    irparams_save = NULL;

    if (save_buffer)
    {
        irparams_save = new irparams_t;
        irparams_save->rawbuf = new uint16_t[bufsize];
        irparams_save->bufsize = bufsize;
    }

    _tolerance = kTolerance;
#if DECODE_HASH
    _unknown_threshold = kUnknownThreshold;
#endif
}

IRrecv::~IRrecv(void)
{
    disableIRIn();
    delete[] g_params.rawbuf;
    g_params.rawbuf = NULL;

    if (irparams_save != NULL)
    {
        delete[] irparams_save->rawbuf;
        delete irparams_save;
        irparams_save = NULL;
    }
}

void IRrecv::enableIRIn(const bool pullup)
{
    pinMode(g_recvpin, pullup ? INPUT_PULLUP : INPUT);
#if IRREMOTE_RA4M2_USE_SYSTICK_RECV
    (void) start_systick();
#endif
    resume();
}

void IRrecv::disableIRIn(void)
{
    pause();
#if IRREMOTE_RA4M2_USE_SYSTICK_RECV
    stop_systick();
#endif
}

void IRrecv::pause(void)
{
#if IRREMOTE_RA4M2_USE_SYSTICK_RECV
    g_capture_enabled = false;
#endif
    g_params.rcvstate = kStopState;
    g_params.rawlen = 0;
    g_params.overflow = false;
}

void IRrecv::resume(void)
{
    g_params.rcvstate = kIdleState;
    g_params.rawlen = 0;
    g_params.overflow = false;
#if IRREMOTE_RA4M2_USE_SYSTICK_RECV
    g_last_level = digitalRead(g_recvpin);
    g_last_edge_us = micros();
    g_capture_enabled = g_systick_running;
#endif
}

void IRrecv::copyIrParams(atomic_irparams_t *src, irparams_t *dst)
{
    uint16_t *dst_rawbuf = dst->rawbuf;

    dst->recvpin = src->recvpin;
    dst->rcvstate = src->rcvstate;
    dst->timer = src->timer;
    dst->bufsize = src->bufsize;
    dst->rawlen = src->rawlen;
    dst->overflow = src->overflow;
    dst->timeout = src->timeout;
    dst->rawbuf = dst_rawbuf;

    for (uint16_t i = 0; i < dst->bufsize; i++)
    {
        dst->rawbuf[i] = src->rawbuf[i];
    }
}

uint16_t IRrecv::getBufSize(void)
{
    return g_params.bufsize;
}

#if DECODE_HASH
void IRrecv::setUnknownThreshold(const uint16_t length)
{
    _unknown_threshold = length;
}
#endif

void IRrecv::setTolerance(const uint8_t percent)
{
    _tolerance = std::min(percent, static_cast<uint8_t>(100));
}

uint8_t IRrecv::getTolerance(void)
{
    return _tolerance;
}

bool IRrecv::decode(decode_results *results, irparams_t *save,
                    uint8_t max_skip, uint16_t noise_floor)
{
    (void) noise_floor;

#if IRREMOTE_RA4M2_USE_SYSTICK_RECV
    if (g_systick_running)
    {
        if ((g_params.rcvstate != kStopState) || (g_params.rawlen == 0U))
        {
            return false;
        }
    }
    else
#endif
    if (g_params.rcvstate != kStopState)
    {
        if (!capture_polling())
        {
            return false;
        }
    }

    if (!g_params.overflow && (g_params.rawlen < g_params.bufsize))
    {
        g_params.rawbuf[g_params.rawlen] = 0;
    }

    bool resumed = false;

    if (save == NULL)
    {
        save = irparams_save;
    }

    if (save == NULL)
    {
        results->rawbuf = g_params.rawbuf;
        results->rawlen = g_params.rawlen;
        results->overflow = g_params.overflow;
    }
    else
    {
        copyIrParams(&g_params, save);
        resume();
        resumed = true;
        results->rawbuf = save->rawbuf;
        results->rawlen = save->rawlen;
        results->overflow = save->overflow;
    }

    results->decode_type = UNKNOWN;
    results->bits = 0;
    results->value = 0;
    results->address = 0;
    results->command = 0;
    results->repeat = false;

    for (uint16_t offset = kStartOffset;
         offset <= (max_skip * 2U) + kStartOffset;
         offset += 2U)
    {
#if DECODE_AIWA_RC_T501
        if (decodeAiwaRCT501(results, offset))
        {
            return true;
        }
#endif
#if DECODE_SANYO
        if (decodeSanyoLC7461(results, offset))
        {
            return true;
        }
#endif
#if DECODE_CARRIER_AC
        if (decodeCarrierAC(results, offset))
        {
            return true;
        }
#endif
#if DECODE_PIONEER
        if (decodePioneer(results, offset))
        {
            return true;
        }
#endif
#if DECODE_EPSON
        if (decodeEpson(results, offset))
        {
            return true;
        }
#endif
#if DECODE_NEC
        if (decodeNEC(results, offset))
        {
            return true;
        }
#endif
#if DECODE_MILESTAG2
        if (decodeMilestag2(results, offset, kMilesTag2MsgBits) ||
            decodeMilestag2(results, offset, kMilesTag2ShotBits))
        {
            return true;
        }
#endif
#if DECODE_SONY
        if (decodeSony(results, offset))
        {
            return true;
        }
#endif
#if DECODE_MITSUBISHI
        if (decodeMitsubishi(results, offset))
        {
            return true;
        }
#endif
#if DECODE_MITSUBISHI_AC
        if (decodeMitsubishiAC(results, offset))
        {
            return true;
        }
#endif
#if DECODE_MITSUBISHI2
        if (decodeMitsubishi2(results, offset))
        {
            return true;
        }
#endif
#if DECODE_RC5
        if (decodeRC5(results, offset))
        {
            return true;
        }
#endif
#if DECODE_RC6
        if (decodeRC6(results, offset))
        {
            return true;
        }
#endif
#if DECODE_RCMM
        if (decodeRCMM(results, offset))
        {
            return true;
        }
#endif
#if DECODE_FUJITSU_AC
        if (decodeFujitsuAC(results, offset))
        {
            return true;
        }
#endif
#if DECODE_DENON
        if (decodeDenon(results, offset, kDenon48Bits) ||
            decodeDenon(results, offset, kDenonBits) ||
            decodeDenon(results, offset, kDenonLegacyBits))
        {
            return true;
        }
#endif
#if DECODE_PANASONIC
        if (decodePanasonic(results, offset) ||
            decodePanasonic(results, offset, kPanasonic40Bits, true, kPanasonic40Manufacturer))
        {
            return true;
        }
#endif
#if DECODE_LG
        if (decodeLG(results, offset, kLgBits, true) ||
            decodeLG(results, offset, kLg32Bits, true))
        {
            return true;
        }
#endif
#if DECODE_GICABLE
        if (decodeGICable(results, offset))
        {
            return true;
        }
#endif
#if DECODE_JVC
        if (decodeJVC(results, offset))
        {
            return true;
        }
#endif
#if DECODE_SAMSUNG
        if (decodeSAMSUNG(results, offset))
        {
            return true;
        }
#endif
#if DECODE_SAMSUNG36
        if (decodeSamsung36(results, offset))
        {
            return true;
        }
#endif
#if DECODE_WHYNTER
        if (decodeWhynter(results, offset))
        {
            return true;
        }
#endif
#if DECODE_DISH
        if (decodeDISH(results, offset))
        {
            return true;
        }
#endif
#if DECODE_SHARP
        if (decodeSharp(results, offset))
        {
            return true;
        }
#endif
#if DECODE_BOSCH144
        if (decodeBosch144(results, offset))
        {
            return true;
        }
#endif
#if DECODE_MIDEA
        if (decodeMidea(results, offset))
        {
            return true;
        }
#endif
#if DECODE_GREE
        if (decodeGree(results, offset))
        {
            return true;
        }
#endif
#if DECODE_COOLIX
        if (decodeCOOLIX(results, offset))
        {
            return true;
        }
#endif
#if DECODE_NIKAI
        if (decodeNikai(results, offset))
        {
            return true;
        }
#endif
#if DECODE_KELVINATOR
        if (decodeKelvinator(results, offset))
        {
            return true;
        }
#endif
#if DECODE_DAIKIN
        if (decodeDaikin(results, offset))
        {
            return true;
        }
#endif
#if DECODE_DAIKIN2
        if (decodeDaikin2(results, offset))
        {
            return true;
        }
#endif
#if DECODE_DAIKIN216
        if (decodeDaikin216(results, offset))
        {
            return true;
        }
#endif
#if DECODE_TOSHIBA_AC
        if (decodeToshibaAC(results, offset) ||
            decodeToshibaAC(results, offset, kToshibaACBitsLong) ||
            decodeToshibaAC(results, offset, kToshibaACBitsShort))
        {
            return true;
        }
#endif
#if DECODE_INAX
        if (decodeInax(results, offset))
        {
            return true;
        }
#endif
#if DECODE_MAGIQUEST
        if (decodeMagiQuest(results, offset))
        {
            return true;
        }
#endif
#if DECODE_NEC
        if (decodeNEC(results, offset, kNECBits, false))
        {
            results->decode_type = NEC_LIKE;
            return true;
        }
#endif
#if DECODE_LASERTAG
        if (decodeLasertag(results, offset))
        {
            return true;
        }
#endif
#if DECODE_HAIER_AC
        if (decodeHaierAC(results, offset))
        {
            return true;
        }
#endif
#if DECODE_HAIER_AC_YRW02
        if (decodeHaierACYRW02(results, offset))
        {
            return true;
        }
#endif
#if DECODE_HAIER_AC176
        if (decodeHaierAC176(results, offset))
        {
            return true;
        }
#endif
#if DECODE_HITACHI_AC424
        if (decodeHitachiAc424(results, offset, kHitachiAc424Bits))
        {
            return true;
        }
#endif
#if DECODE_GOODWEATHER
        if (decodeGoodweather(results, offset))
        {
            return true;
        }
#endif
#if DECODE_MITSUBISHI136
        if (decodeMitsubishi136(results, offset))
        {
            return true;
        }
#endif
#if DECODE_HITACHI_AC3
        if (decodeHitachiAc3(results, offset, kHitachiAc3Bits) ||
            decodeHitachiAc3(results, offset, kHitachiAc3Bits - 4U * 8U) ||
            decodeHitachiAc3(results, offset, kHitachiAc3Bits - 6U * 8U) ||
            decodeHitachiAc3(results, offset, kHitachiAc3MinBits + 2U * 8U) ||
            decodeHitachiAc3(results, offset, kHitachiAc3MinBits))
        {
            return true;
        }
#endif
#if DECODE_HITACHI_AC344
        if (decodeHitachiAC(results, offset, kHitachiAc344Bits, true, false))
        {
            return true;
        }
#endif
#if DECODE_HITACHI_AC264
        if (decodeHitachiAC(results, offset, kHitachiAc264Bits, true, false))
        {
            return true;
        }
#endif
#if DECODE_HITACHI_AC296
        if (decodeHitachiAc296(results, offset, kHitachiAc296Bits, true))
        {
            return true;
        }
#endif
#if DECODE_HITACHI_AC2
        if (decodeHitachiAC(results, offset, kHitachiAc2Bits))
        {
            return true;
        }
#endif
#if DECODE_HITACHI_AC
        if (decodeHitachiAC(results, offset, kHitachiAcBits))
        {
            return true;
        }
#endif
#if DECODE_HITACHI_AC1
        if (decodeHitachiAC(results, offset, kHitachiAc1Bits))
        {
            return true;
        }
#endif
#if DECODE_LUTRON
        if (decodeLutron(results, offset))
        {
            return true;
        }
#endif
#if DECODE_MWM
        if (decodeMWM(results, offset))
        {
            return true;
        }
#endif
#if DECODE_LEGOPF
        if (decodeLegoPf(results, offset))
        {
            return true;
        }
#endif
#if DECODE_MITSUBISHIHEAVY
        if (decodeMitsubishiHeavy(results, offset, kMitsubishiHeavy152Bits) ||
            decodeMitsubishiHeavy(results, offset, kMitsubishiHeavy88Bits))
        {
            return true;
        }
#endif
#if DECODE_ARGO
        if (decodeArgoWREM3(results, offset, kArgo3AcControlStateLength * 8U, true) ||
            decodeArgoWREM3(results, offset, kArgo3iFeelReportStateLength * 8U, true) ||
            decodeArgoWREM3(results, offset, kArgo3ConfigStateLength * 8U, true) ||
            decodeArgoWREM3(results, offset, kArgo3TimerStateLength * 8U, true) ||
            decodeArgo(results, offset, kArgoBits) ||
            decodeArgo(results, offset, kArgoShortBits, false))
        {
            return true;
        }
#endif
#if DECODE_ELECTRA_AC
        if (decodeElectraAC(results, offset))
        {
            return true;
        }
#endif
#if DECODE_PANASONIC_AC
        if (decodePanasonicAC(results, offset) ||
            decodePanasonicAC(results, offset, kPanasonicAcShortBits))
        {
            return true;
        }
#endif
#if DECODE_MITSUBISHI112
        if (decodeMitsubishi112(results, offset))
        {
            return true;
        }
#endif
#if DECODE_TECO
        if (decodeTeco(results, offset))
        {
            return true;
        }
#endif
#if DECODE_SYMPHONY
        if (decodeSymphony(results, offset))
        {
            return true;
        }
#endif
#if DECODE_DAIKIN64
        if (decodeDaikin64(results, offset))
        {
            return true;
        }
#endif
#if DECODE_AIRWELL
        if (decodeAirwell(results, offset))
        {
            return true;
        }
#endif
#if DECODE_DELONGHI_AC
        if (decodeDelonghiAc(results, offset))
        {
            return true;
        }
#endif
#if DECODE_DOSHISHA
        if (decodeDoshisha(results, offset))
        {
            return true;
        }
#endif
#if DECODE_TRUMA
        if (decodeTruma(results, offset))
        {
            return true;
        }
#endif
#if DECODE_MULTIBRACKETS
        if (decodeMultibrackets(results, offset))
        {
            return true;
        }
#endif
#if DECODE_CARRIER_AC40
        if (decodeCarrierAC40(results, offset))
        {
            return true;
        }
#endif
#if DECODE_CARRIER_AC64
        if (decodeCarrierAC64(results, offset))
        {
            return true;
        }
#endif
#if DECODE_TECHNIBEL_AC
        if (decodeTechnibelAc(results, offset))
        {
            return true;
        }
#endif
#if DECODE_CORONA_AC
        if (decodeCoronaAc(results, offset))
        {
            return true;
        }
#endif
#if DECODE_MIDEA24
        if (decodeMidea24(results, offset))
        {
            return true;
        }
#endif
#if DECODE_SANYO_AC
        if (decodeSanyoAc(results, offset))
        {
            return true;
        }
#endif
#if DECODE_ZEPEAL
        if (decodeZepeal(results, offset))
        {
            return true;
        }
#endif
#if DECODE_VOLTAS
        if (decodeVoltas(results, offset))
        {
            return true;
        }
#endif
#if DECODE_METZ
        if (decodeMetz(results, offset))
        {
            return true;
        }
#endif
#if DECODE_TRANSCOLD
        if (decodeTranscold(results, offset))
        {
            return true;
        }
#endif
#if DECODE_MIRAGE
        if (decodeMirage(results, offset))
        {
            return true;
        }
#endif
#if DECODE_PANASONIC_AC32
        if (decodePanasonicAC32(results, offset, kPanasonicAc32Bits) ||
            decodePanasonicAC32(results, offset, kPanasonicAc32Bits / 2U))
        {
            return true;
        }
#endif
#if DECODE_ECOCLIM
        if (decodeEcoclim(results, offset, kEcoclimBits) ||
            decodeEcoclim(results, offset, kEcoclimShortBits))
        {
            return true;
        }
#endif
#if DECODE_ELITESCREENS
        if (decodeElitescreens(results, offset))
        {
            return true;
        }
#endif
#if DECODE_XMP
        if (decodeXmp(results, offset, kXmpBits))
        {
            return true;
        }
#endif
#if DECODE_TEKNOPOINT
        if (decodeTeknopoint(results, offset))
        {
            return true;
        }
#endif
#if DECODE_KELON168
        if (decodeKelon168(results, offset))
        {
            return true;
        }
#endif
#if DECODE_KELON
        if (decodeKelon(results, offset))
        {
            return true;
        }
#endif
#if DECODE_SANYO_AC88
        if (decodeSanyoAc88(results, offset))
        {
            return true;
        }
#endif
#if DECODE_BOSE
        if (decodeBose(results, offset))
        {
            return true;
        }
#endif
#if DECODE_ARRIS
        if (decodeArris(results, offset))
        {
            return true;
        }
#endif
#if DECODE_RHOSS
        if (decodeRhoss(results, offset))
        {
            return true;
        }
#endif
#if DECODE_COOLIX48
        if (decodeCoolix48(results, offset))
        {
            return true;
        }
#endif
#if DECODE_DAIKIN200
        if (decodeDaikin200(results, offset))
        {
            return true;
        }
#endif
#if DECODE_HAIER_AC160
        if (decodeHaierAC160(results, offset))
        {
            return true;
        }
#endif
#if DECODE_CARRIER_AC128
        if (decodeCarrierAC128(results, offset))
        {
            return true;
        }
#endif
#if DECODE_TOTO
        if (decodeToto(results, offset, kTotoLongBits) ||
            decodeToto(results, offset, kTotoShortBits))
        {
            return true;
        }
#endif
#if DECODE_TCL96AC
        if (decodeTcl96Ac(results, offset))
        {
            return true;
        }
#endif
#if DECODE_SANYO_AC152
        if (decodeSanyoAc152(results, offset))
        {
            return true;
        }
#endif
#if DECODE_DAIKIN312
        if (decodeDaikin312(results, offset))
        {
            return true;
        }
#endif
#if DECODE_CLIMABUTLER
        if (decodeClimaButler(results, offset))
        {
            return true;
        }
#endif
#if DECODE_GORENJE
        if (decodeGorenje(results, offset))
        {
            return true;
        }
#endif
#if DECODE_WOWWEE
        if (decodeWowwee(results, offset))
        {
            return true;
        }
#endif
#if DECODE_TROTEC
        if (decodeTrotec(results, offset))
        {
            return true;
        }
#endif
#if DECODE_TROTEC_3550
        if (decodeTrotec3550(results, offset))
        {
            return true;
        }
#endif
#if DECODE_DAIKIN160
        if (decodeDaikin160(results, offset))
        {
            return true;
        }
#endif
#if DECODE_NEOCLIMA
        if (decodeNeoclima(results, offset))
        {
            return true;
        }
#endif
#if DECODE_DAIKIN176
        if (decodeDaikin176(results, offset))
        {
            return true;
        }
#endif
#if DECODE_DAIKIN128
        if (decodeDaikin128(results, offset))
        {
            return true;
        }
#endif
#if DECODE_AMCOR
        if (decodeAmcor(results, offset))
        {
            return true;
        }
#endif
#if DECODE_DAIKIN152
        if (decodeDaikin152(results, offset))
        {
            return true;
        }
#endif
#if DECODE_AIRTON
        if (decodeAirton(results, offset))
        {
            return true;
        }
#endif
#if DECODE_SHARP_AC
        if (decodeSharpAc(results, offset))
        {
            return true;
        }
#endif
#if DECODE_WHIRLPOOL_AC
        if (decodeWhirlpoolAC(results, offset))
        {
            return true;
        }
#endif
#if DECODE_SAMSUNG_AC
        if (decodeSamsungAC(results, offset, kSamsungAcExtendedBits) ||
            decodeSamsungAC(results, offset, kSamsungAcBits))
        {
            return true;
        }
#endif
#if DECODE_VESTEL_AC
        if (decodeVestelAc(results, offset))
        {
            return true;
        }
#endif
#if DECODE_CARRIER_AC84
        if (decodeCarrierAC84(results, offset))
        {
            return true;
        }
#endif
#if DECODE_YORK
        if (decodeYork(results, offset, kYorkBits))
        {
            return true;
        }
#endif
#if DECODE_BLUESTARHEAVY
        if (decodeBluestarHeavy(results, offset, kBluestarHeavyBits))
        {
            return true;
        }
#endif
#if DECODE_EUROM
        if (decodeEurom(results, offset, kEuromBits))
        {
            return true;
        }
#endif
    }

#if DECODE_HASH
    if (decodeHash(results))
    {
        return true;
    }
#endif

    if (!resumed)
    {
        resume();
    }

    return false;
}

uint8_t IRrecv::_validTolerance(const uint8_t percentage)
{
    return (percentage > 100) ? _tolerance : percentage;
}

uint32_t IRrecv::ticksLow(const uint32_t usecs, const uint8_t tolerance,
                          const uint16_t delta)
{
    return static_cast<uint32_t>(std::max(
        static_cast<int32_t>(usecs * (1.0 - _validTolerance(tolerance) / 100.0) - delta),
        static_cast<int32_t>(0)));
}

uint32_t IRrecv::ticksHigh(const uint32_t usecs, const uint8_t tolerance,
                           const uint16_t delta)
{
    return static_cast<uint32_t>(usecs * (1.0 + _validTolerance(tolerance) / 100.0)) + 1 + delta;
}

bool IRrecv::match(uint32_t measured, uint32_t desired, uint8_t tolerance,
                   uint16_t delta)
{
    measured *= kRawTick;
    return (measured >= ticksLow(desired, tolerance, delta)) &&
           (measured <= ticksHigh(desired, tolerance, delta));
}

bool IRrecv::matchAtLeast(uint32_t measured, uint32_t desired,
                          uint8_t tolerance, uint16_t delta)
{
    measured *= kRawTick;

    if (measured == 0)
    {
        return true;
    }

    return measured >= ticksLow(std::min(desired,
                                         static_cast<uint32_t>(MS_TO_USEC(g_params.timeout))),
                                tolerance,
                                delta);
}

bool IRrecv::matchMark(uint32_t measured, uint32_t desired, uint8_t tolerance,
                       int16_t excess)
{
    return match(measured, desired + excess, tolerance);
}

bool IRrecv::matchMarkRange(const uint32_t measured, const uint32_t desired,
                            const uint16_t range, const int16_t excess)
{
    return match(measured, desired + excess, 0, range);
}

bool IRrecv::matchSpace(uint32_t measured, uint32_t desired, uint8_t tolerance,
                        int16_t excess)
{
    return match(measured, desired - excess, tolerance);
}

bool IRrecv::matchSpaceRange(const uint32_t measured, const uint32_t desired,
                             const uint16_t range, const int16_t excess)
{
    return match(measured, desired - excess, 0, range);
}

#if DECODE_HASH
uint16_t IRrecv::compare(const uint16_t oldval, const uint16_t newval)
{
    if (newval < oldval * 0.8)
    {
        return 0;
    }

    if (oldval < newval * 0.8)
    {
        return 2;
    }

    return 1;
}

bool IRrecv::decodeHash(decode_results *results)
{
    if (results->rawlen < _unknown_threshold)
    {
        return false;
    }

    uint32_t hash = kFnvBasis32;

    for (uint16_t i = 1; i < results->rawlen - 2U; i++)
    {
        const uint16_t value = compare(results->rawbuf[i], results->rawbuf[i + 2U]);
        hash = (hash * kFnvPrime32) ^ value;
    }

    results->value = hash;
    results->bits = 32;
    results->decode_type = UNKNOWN;
    results->address = 0;
    results->command = 0;
    return true;
}
#endif

match_result_t IRrecv::matchData(atomic_uint16_t *data_ptr,
                                 const uint16_t nbits,
                                 const uint16_t onemark,
                                 const uint32_t onespace,
                                 const uint16_t zeromark,
                                 const uint32_t zerospace,
                                 const uint8_t tolerance,
                                 const int16_t excess,
                                 const bool MSBfirst,
                                 const bool expectlastspace)
{
    match_result_t result;
    result.success = false;
    result.data = 0;

    if (expectlastspace)
    {
        for (result.used = 0; result.used < nbits * 2U;
             result.used += 2U, data_ptr += 2U)
        {
            if (matchMark(*data_ptr, onemark, tolerance, excess) &&
                matchSpace(*(data_ptr + 1), onespace, tolerance, excess))
            {
                result.data = (result.data << 1U) | 1U;
            }
            else if (matchMark(*data_ptr, zeromark, tolerance, excess) &&
                     matchSpace(*(data_ptr + 1), zerospace, tolerance, excess))
            {
                result.data <<= 1U;
            }
            else
            {
                if (!MSBfirst)
                {
                    result.data = reverseBits(result.data, result.used / 2U);
                }

                return result;
            }
        }

        result.success = true;
    }
    else
    {
        result = matchData(data_ptr,
                           static_cast<uint16_t>(nbits ? nbits - 1U : 0U),
                           onemark, onespace,
                           zeromark, zerospace, tolerance, excess, true, true);

        if (result.success)
        {
            if (matchMark(*(data_ptr + result.used), onemark, tolerance, excess))
            {
                result.data = (result.data << 1U) | 1U;
            }
            else if (matchMark(*(data_ptr + result.used), zeromark, tolerance, excess))
            {
                result.data <<= 1U;
            }
            else
            {
                result.success = false;
            }

            if (result.success)
            {
                result.used++;
            }
        }
    }

    if (!MSBfirst)
    {
        result.data = reverseBits(result.data, nbits);
    }

    return result;
}

uint16_t IRrecv::matchBytes(atomic_uint16_t *data_ptr, uint8_t *result_ptr,
                            const uint16_t remaining, const uint16_t nbytes,
                            const uint16_t onemark, const uint32_t onespace,
                            const uint16_t zeromark, const uint32_t zerospace,
                            const uint8_t tolerance, const int16_t excess,
                            const bool MSBfirst, const bool expectlastspace)
{
    if (static_cast<uint32_t>(remaining) + (expectlastspace ? 1UL : 0UL) <
        (static_cast<uint32_t>(nbytes) * 8UL * 2UL) + 1UL)
    {
        return 0;
    }

    uint16_t offset = 0;

    for (uint16_t byte_pos = 0; byte_pos < nbytes; byte_pos++)
    {
        const bool lastspace = (byte_pos + 1U == nbytes) ? expectlastspace : true;
        match_result_t result = matchData(data_ptr + offset, 8, onemark, onespace,
                                          zeromark, zerospace, tolerance, excess,
                                          MSBfirst, lastspace);

        if (!result.success)
        {
            return 0;
        }

        result_ptr[byte_pos] = static_cast<uint8_t>(result.data);
        offset += result.used;
    }

    return offset;
}

uint16_t IRrecv::_matchGeneric(atomic_uint16_t *data_ptr,
                               uint64_t *result_bits_ptr,
                               uint8_t *result_bytes_ptr,
                               const bool use_bits,
                               const uint16_t remaining,
                               const uint16_t nbits,
                               const uint16_t hdrmark,
                               const uint32_t hdrspace,
                               const uint16_t onemark,
                               const uint32_t onespace,
                               const uint16_t zeromark,
                               const uint32_t zerospace,
                               const uint16_t footermark,
                               const uint32_t footerspace,
                               const bool atleast,
                               const uint8_t tolerance,
                               const int16_t excess,
                               const bool MSBfirst)
{
    if (!use_bits && (nbits % 8U != 0U))
    {
        return 0;
    }

    const bool expectspace = footermark || (onespace != zerospace);
    uint16_t min_remaining = static_cast<uint16_t>(nbits * 2U - (expectspace ? 0U : 1U));

    if (hdrmark)
    {
        min_remaining++;
    }
    if (hdrspace)
    {
        min_remaining++;
    }
    if (footermark)
    {
        min_remaining++;
    }

    if (remaining < min_remaining)
    {
        return 0;
    }

    uint16_t offset = 0;

    if (hdrmark && !matchMark(*(data_ptr + offset++), hdrmark, tolerance, excess))
    {
        return 0;
    }
    if (hdrspace && !matchSpace(*(data_ptr + offset++), hdrspace, tolerance, excess))
    {
        return 0;
    }

    if (use_bits)
    {
        match_result_t result = IRrecv::matchData(data_ptr + offset, nbits,
                                                  onemark, onespace,
                                                  zeromark, zerospace,
                                                  tolerance, excess,
                                                  MSBfirst, expectspace);
        if (!result.success)
        {
            return 0;
        }

        *result_bits_ptr = result.data;
        offset += result.used;
    }
    else
    {
        uint16_t used = IRrecv::matchBytes(data_ptr + offset, result_bytes_ptr,
                                           remaining - offset, nbits / 8U,
                                           onemark, onespace,
                                           zeromark, zerospace,
                                           tolerance, excess,
                                           MSBfirst, expectspace);

        if (!used)
        {
            return 0;
        }

        offset += used;
    }

    if (footermark && !matchMark(*(data_ptr + offset++), footermark, tolerance, excess))
    {
        return 0;
    }

    if (footerspace && (offset < remaining))
    {
        if (atleast)
        {
            if (!matchAtLeast(*(data_ptr + offset), footerspace, tolerance, excess))
            {
                return 0;
            }
        }
        else
        {
            if (!matchSpace(*(data_ptr + offset), footerspace, tolerance, excess))
            {
                return 0;
            }
        }

        offset++;
    }

    return offset;
}

uint16_t IRrecv::matchGeneric(atomic_uint16_t *data_ptr,
                              uint64_t *result_ptr,
                              const uint16_t remaining,
                              const uint16_t nbits,
                              const uint16_t hdrmark,
                              const uint32_t hdrspace,
                              const uint16_t onemark,
                              const uint32_t onespace,
                              const uint16_t zeromark,
                              const uint32_t zerospace,
                              const uint16_t footermark,
                              const uint32_t footerspace,
                              const bool atleast,
                              const uint8_t tolerance,
                              const int16_t excess,
                              const bool MSBfirst)
{
    return _matchGeneric(data_ptr, result_ptr, NULL, true, remaining, nbits,
                         hdrmark, hdrspace, onemark, onespace, zeromark,
                         zerospace, footermark, footerspace, atleast,
                         tolerance, excess, MSBfirst);
}

uint16_t IRrecv::matchGeneric(atomic_uint16_t *data_ptr,
                              uint8_t *result_ptr,
                              const uint16_t remaining,
                              const uint16_t nbits,
                              const uint16_t hdrmark,
                              const uint32_t hdrspace,
                              const uint16_t onemark,
                              const uint32_t onespace,
                              const uint16_t zeromark,
                              const uint32_t zerospace,
                              const uint16_t footermark,
                              const uint32_t footerspace,
                              const bool atleast,
                              const uint8_t tolerance,
                              const int16_t excess,
                              const bool MSBfirst)
{
    return _matchGeneric(data_ptr, NULL, result_ptr, false, remaining, nbits,
                         hdrmark, hdrspace, onemark, onespace, zeromark,
                         zerospace, footermark, footerspace, atleast,
                         tolerance, excess, MSBfirst);
}

uint16_t IRrecv::matchGenericConstBitTime(atomic_uint16_t *data_ptr,
                                          uint64_t *result_ptr,
                                          const uint16_t remaining,
                                          const uint16_t nbits,
                                          const uint16_t hdrmark,
                                          const uint32_t hdrspace,
                                          const uint16_t one,
                                          const uint32_t zero,
                                          const uint16_t footermark,
                                          const uint32_t footerspace,
                                          const bool atleast,
                                          const uint8_t tolerance,
                                          const int16_t excess,
                                          const bool MSBfirst)
{
    const uint16_t zero_mark = static_cast<uint16_t>(zero);

    if (footermark)
    {
        return _matchGeneric(data_ptr, result_ptr, NULL, true, remaining, nbits,
                             hdrmark, hdrspace, one, zero, zero_mark, one,
                             footermark, footerspace, atleast,
                             tolerance, excess, MSBfirst);
    }

    uint64_t result = 0;
    const uint16_t bits = (nbits > 0U) ? static_cast<uint16_t>(nbits - 1U) : 0U;
    uint16_t offset = _matchGeneric(data_ptr, &result, NULL, true, remaining, bits,
                                    hdrmark, hdrspace, one, zero, zero_mark, one,
                                    0, 0, false, tolerance, excess, true);
    if (!offset || remaining <= offset)
    {
        return 0;
    }

    result <<= 1U;
    bool last_bit = false;

    if (matchMark(*(data_ptr + offset), one, tolerance, excess))
    {
        last_bit = true;
        result |= 1U;
    }
    else if (!matchMark(*(data_ptr + offset), zero, tolerance, excess))
    {
        return 0;
    }

    offset++;
    const uint32_t expected_space = (last_bit ? zero : one) + footerspace;

    if (remaining > offset)
    {
        if (atleast)
        {
            if (!matchAtLeast(*(data_ptr + offset), expected_space, tolerance, excess))
            {
                return 0;
            }
        }
        else if (!matchSpace(*(data_ptr + offset), expected_space, tolerance))
        {
            return 0;
        }

        offset++;
    }

    if (!MSBfirst)
    {
        result = reverseBits(result, nbits);
    }

    *result_ptr = result;
    return offset;
}

uint16_t IRrecv::matchManchester(atomic_const_uint16_t *data_ptr,
                                 uint64_t *result_ptr,
                                 const uint16_t remaining,
                                 const uint16_t nbits,
                                 const uint16_t hdrmark,
                                 const uint32_t hdrspace,
                                 const uint16_t half_period,
                                 const uint16_t footermark,
                                 const uint32_t footerspace,
                                 const bool atleast,
                                 const uint8_t tolerance,
                                 const int16_t excess,
                                 const bool MSBfirst,
                                 const bool GEThomas)
{
    uint16_t offset = 0;
    uint16_t bank = 0;
    uint16_t entry = 0;
    uint16_t min_remaining = nbits;

    if (hdrmark)
    {
        min_remaining++;
    }
    if (hdrspace)
    {
        min_remaining++;
    }
    if (footermark)
    {
        min_remaining++;
    }

    if (remaining < min_remaining)
    {
        return 0;
    }

    if (hdrmark)
    {
        entry = *(data_ptr + offset++);
        if (!hdrspace)
        {
            if (matchMark(entry, hdrmark + half_period, tolerance, excess))
            {
                bank = static_cast<uint16_t>(entry * kRawTick - hdrmark);
            }
            else if (!matchMark(entry, hdrmark, tolerance, excess))
            {
                return 0;
            }
        }
        else if (!matchMark(entry, hdrmark, tolerance, excess))
        {
            return 0;
        }
    }

    if (hdrspace)
    {
        entry = *(data_ptr + offset++);
        if (matchSpace(entry, hdrspace + half_period, tolerance, excess))
        {
            bank = static_cast<uint16_t>(entry * kRawTick - hdrspace);
        }
        else if (!matchSpace(entry, hdrspace, tolerance, excess))
        {
            return 0;
        }
    }

    if (!match(bank / kRawTick, half_period, tolerance, excess))
    {
        bank = 0;
    }

    const uint16_t used = matchManchesterData(data_ptr + offset, result_ptr,
                                              remaining - offset, nbits,
                                              half_period, bank, tolerance,
                                              excess, MSBfirst, GEThomas);
    if (!used)
    {
        return 0;
    }

    offset += used;

    if (footermark)
    {
        if (offset >= remaining)
        {
            return 0;
        }

        if (!(matchMark(*(data_ptr + offset), footermark + half_period,
                        tolerance, excess) ||
              matchMark(*(data_ptr + offset), footermark, tolerance, excess)))
        {
            return 0;
        }

        offset++;
    }

    if (footerspace && offset < remaining)
    {
        if (atleast)
        {
            if (!matchAtLeast(*(data_ptr + offset), footerspace, tolerance, excess))
            {
                return 0;
            }
        }
        else if (!matchSpace(*(data_ptr + offset), footerspace, tolerance, excess) &&
                 !matchSpace(*(data_ptr + offset), footerspace + half_period,
                             tolerance, excess))
        {
            return 0;
        }

        offset++;
    }

    return offset;
}

uint16_t IRrecv::matchManchesterData(atomic_const_uint16_t *data_ptr,
                                     uint64_t *result_ptr,
                                     const uint16_t remaining,
                                     const uint16_t nbits,
                                     const uint16_t half_period,
                                     const uint16_t starting_balance,
                                     const uint8_t tolerance,
                                     const int16_t excess,
                                     const bool MSBfirst,
                                     const bool GEThomas)
{
    uint16_t offset = 0;
    uint64_t data = 0;
    uint16_t nr_half_periods = 0;
    const uint16_t expected_half_periods = nbits * 2U;
    bool current_bit = starting_balance ? !GEThomas : GEThomas;
    const uint16_t raw_half_period = half_period / kRawTick;

    if (remaining < nbits)
    {
        return 0;
    }

    uint16_t bank = starting_balance / kRawTick;

    while (((offset < remaining) || bank) &&
           (nr_half_periods < expected_half_periods))
    {
        if (!bank)
        {
            bank = *(data_ptr + offset++);
        }

        if (!match(bank, half_period, tolerance, excess))
        {
            return 0;
        }

        nr_half_periods++;

        if (offset < remaining)
        {
            bank = *(data_ptr + offset++);
        }
        else if (offset == remaining)
        {
            bank = raw_half_period;
        }
        else
        {
            return 0;
        }

        data <<= 1U;
        data |= current_bit ? 1U : 0U;

        if (match(bank, half_period * 2U, tolerance, excess))
        {
            current_bit = !current_bit;
            bank -= raw_half_period;
        }
        else if (match(bank, half_period, tolerance, excess))
        {
            bank = 0;
        }
        else if ((nr_half_periods == expected_half_periods - 1U) &&
                 matchAtLeast(bank, half_period, tolerance, excess))
        {
            bank = 0;
            offset--;
        }
        else
        {
            return 0;
        }

        nr_half_periods++;
    }

    if (!MSBfirst)
    {
        data = reverseBits(data, nbits);
    }

    *result_ptr = GETBITS64(data, 0, nbits);
    return offset;
}
