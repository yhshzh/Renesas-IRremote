#include "app/aircon_ir_service.h"

#include <stdio.h>
#include <string.h>

#include "IRutils.h"
#include "app/app_json.h"
#include "ir_Coolix.h"

namespace
{
const char *bool_text(bool value)
{
    return value ? "true" : "false";
}

void float1_text(float value, char *out, size_t out_size)
{
    int scaled = static_cast<int>((value * 10.0f) + ((value >= 0.0f) ? 0.5f : -0.5f));
    int whole = scaled / 10;
    int fraction = scaled % 10;

    if (fraction < 0)
    {
        fraction = -fraction;
    }

    snprintf(out, out_size, "%d.%d", whole, fraction);
}

void value_hex(uint64_t value, char *out, size_t out_size)
{
    const uint32_t upper = static_cast<uint32_t>(value >> 32U);
    const uint32_t lower = static_cast<uint32_t>(value & 0xFFFFFFFFULL);

    if (upper != 0U)
    {
        snprintf(out, out_size, "0x%lx%08lx", static_cast<unsigned long>(upper), static_cast<unsigned long>(lower));
    }
    else
    {
        snprintf(out, out_size, "0x%lx", static_cast<unsigned long>(lower));
    }
}

const char *mode_name(stdAc::opmode_t mode)
{
    switch (mode)
    {
        case stdAc::opmode_t::kOff:
            return "off";
        case stdAc::opmode_t::kAuto:
            return "auto";
        case stdAc::opmode_t::kCool:
            return "cool";
        case stdAc::opmode_t::kHeat:
            return "heat";
        case stdAc::opmode_t::kDry:
            return "dry";
        case stdAc::opmode_t::kFan:
            return "fan";
        default:
            return "auto";
    }
}

stdAc::opmode_t parse_mode(const char *value, stdAc::opmode_t current)
{
    if (0 == irremote_ra_strcasecmp(value, "off"))
    {
        return stdAc::opmode_t::kOff;
    }

    return IRac::strToOpmode(value, current);
}

const char *fan_name(stdAc::fanspeed_t fan)
{
    switch (fan)
    {
        case stdAc::fanspeed_t::kAuto:
            return "auto";
        case stdAc::fanspeed_t::kMin:
            return "min";
        case stdAc::fanspeed_t::kLow:
            return "low";
        case stdAc::fanspeed_t::kMedium:
            return "medium";
        case stdAc::fanspeed_t::kHigh:
            return "high";
        case stdAc::fanspeed_t::kMax:
            return "max";
        case stdAc::fanspeed_t::kMediumHigh:
            return "mediumHigh";
        default:
            return "auto";
    }
}

const char *swingv_name(stdAc::swingv_t swing)
{
    switch (swing)
    {
        case stdAc::swingv_t::kOff:
            return "off";
        case stdAc::swingv_t::kAuto:
            return "auto";
        case stdAc::swingv_t::kHighest:
            return "highest";
        case stdAc::swingv_t::kHigh:
            return "high";
        case stdAc::swingv_t::kMiddle:
            return "middle";
        case stdAc::swingv_t::kLow:
            return "low";
        case stdAc::swingv_t::kLowest:
            return "lowest";
        case stdAc::swingv_t::kUpperMiddle:
            return "upperMiddle";
        default:
            return "off";
    }
}

const char *swingh_name(stdAc::swingh_t swing)
{
    switch (swing)
    {
        case stdAc::swingh_t::kOff:
            return "off";
        case stdAc::swingh_t::kAuto:
            return "auto";
        case stdAc::swingh_t::kLeftMax:
            return "leftMax";
        case stdAc::swingh_t::kLeft:
            return "left";
        case stdAc::swingh_t::kMiddle:
            return "middle";
        case stdAc::swingh_t::kRight:
            return "right";
        case stdAc::swingh_t::kRightMax:
            return "rightMax";
        case stdAc::swingh_t::kWide:
            return "wide";
        default:
            return "off";
    }
}

bool is_coolix_family(decode_type_t protocol)
{
    return (COOLIX == protocol) || (COOLIX48 == protocol);
}
}

AirconIrService::AirconIrService() :
    receiver_(APP_IR_RX_PIN, APP_IR_RECV_BUFFER_SIZE, APP_IR_RECV_TIMEOUT_MS),
    ac_sender_(APP_IR_TX_PIN),
    results_(),
    current_state_(),
    previous_state_(),
    last_frame_(),
    learning_(false),
    receiver_enabled_(false),
    current_state_valid_(false),
    previous_state_valid_(false)
{
    last_frame_.decode_type = UNKNOWN;
    last_frame_.value = 0ULL;
    last_frame_.bits = 0U;
    last_frame_.rawlen = 0U;
    last_frame_.overflow = false;
}

void AirconIrService::begin()
{
    IRac::initState(&current_state_);
    IRac::initState(&previous_state_);
}

void AirconIrService::start_learning()
{
    if (!receiver_enabled_)
    {
        receiver_.enableIRIn(true);
        receiver_enabled_ = true;
    }
    else
    {
        receiver_.resume();
    }

    learning_ = true;
}

void AirconIrService::stop_learning()
{
    learning_ = false;
    if (receiver_enabled_)
    {
        receiver_.disableIRIn();
        receiver_enabled_ = false;
    }
}

bool AirconIrService::is_learning() const
{
    return learning_;
}

bool AirconIrService::poll()
{
    if (!learning_)
    {
        return false;
    }

    if (!receiver_.decode(&results_))
    {
        return false;
    }

    last_frame_.decode_type = results_.decode_type;
    last_frame_.value = results_.value;
    last_frame_.bits = results_.bits;
    last_frame_.rawlen = results_.rawlen;
    last_frame_.overflow = results_.overflow;

    stdAc::state_t decoded_state;
    if (decode_ac_state(results_, &decoded_state))
    {
        current_state_ = decoded_state;
        previous_state_ = decoded_state;
        current_state_valid_ = true;
        previous_state_valid_ = true;
    }

    stop_learning();
    return true;
}

bool AirconIrService::apply_state_json(const char *json, char *error, size_t error_size)
{
    stdAc::state_t next_state = current_state_;
    if (!current_state_valid_)
    {
        IRac::initState(&next_state);
    }

    char protocol[40];
    if (app_json_get_string(json, "protocol", protocol, sizeof(protocol)))
    {
        next_state.protocol = strToDecodeType(protocol);
    }

    if (UNKNOWN == next_state.protocol)
    {
        set_error(error, error_size, "no_protocol");
        return false;
    }

    if (!IRac::isProtocolSupported(next_state.protocol))
    {
        set_error(error, error_size, "unsupported_protocol");
        return false;
    }

    update_field_strings(json, &next_state);
    update_field_numbers(json, &next_state);
    update_field_bools(json, &next_state);

    const stdAc::state_t rollback_state = current_state_;
    const bool rollback_state_valid = current_state_valid_;

    current_state_ = next_state;
    current_state_valid_ = true;
    if (!send_current_state(error, error_size))
    {
        current_state_ = rollback_state;
        current_state_valid_ = rollback_state_valid;
        return false;
    }

    return true;
}

void AirconIrService::state_json(char *out, size_t out_size, const char *kind) const
{
    if ((NULL == out) || (0U == out_size))
    {
        return;
    }

    char degrees[16];
    char sensor[16];
    char value[24];
    float1_text(current_state_.degrees, degrees, sizeof(degrees));
    float1_text(current_state_.sensorTemperature, sensor, sizeof(sensor));
    value_hex(last_frame_.value, value, sizeof(value));

    const String frame_protocol = typeToString(last_frame_.decode_type);
    const String ac_protocol = typeToString(current_state_.protocol);
    snprintf(out,
             out_size,
             "{\"kind\":\"%s\",\"learning\":%s,"
             "\"frame\":{\"protocol\":\"%s\",\"bits\":%u,\"value_hex\":\"%s\","
             "\"rawlen\":%u,\"overflow\":%s},"
             "\"ac\":{\"valid\":%s,\"protocol\":\"%s\",\"model\":%d,"
             "\"power\":%s,\"mode\":\"%s\",\"degrees\":%s,\"celsius\":%s,"
             "\"fanspeed\":\"%s\",\"swingv\":\"%s\",\"swingh\":\"%s\","
             "\"quiet\":%s,\"turbo\":%s,\"econo\":%s,\"light\":%s,"
             "\"filter\":%s,\"clean\":%s,\"beep\":%s,\"sleep\":%d,"
             "\"clock\":%d,\"command\":%d,\"ifeel\":%s,"
             "\"sensor_temperature\":%s}}\n",
             (NULL == kind) ? "state" : kind,
             bool_text(learning_),
             frame_protocol.c_str(),
             static_cast<unsigned int>(last_frame_.bits),
             value,
             static_cast<unsigned int>(last_frame_.rawlen),
             bool_text(last_frame_.overflow),
             bool_text(current_state_valid_),
             ac_protocol.c_str(),
             static_cast<int>(current_state_.model),
             bool_text(current_state_.power),
             mode_name(current_state_.mode),
             degrees,
             bool_text(current_state_.celsius),
             fan_name(current_state_.fanspeed),
             swingv_name(current_state_.swingv),
             swingh_name(current_state_.swingh),
             bool_text(current_state_.quiet),
             bool_text(current_state_.turbo),
             bool_text(current_state_.econo),
             bool_text(current_state_.light),
             bool_text(current_state_.filter),
             bool_text(current_state_.clean),
             bool_text(current_state_.beep),
             static_cast<int>(current_state_.sleep),
             static_cast<int>(current_state_.clock),
             static_cast<int>(current_state_.command),
             bool_text(current_state_.iFeel),
             sensor);
}

bool AirconIrService::decode_ac_state(const decode_results &results, stdAc::state_t *state)
{
    if (NULL == state)
    {
        return false;
    }

    const stdAc::state_t *previous = NULL;
    if (previous_state_valid_)
    {
        if ((previous_state_.protocol == results.decode_type) ||
            (is_coolix_family(previous_state_.protocol) && is_coolix_family(results.decode_type)))
        {
            previous = &previous_state_;
        }
    }

    if (IRAcUtils::decodeToState(&results, state, previous))
    {
        return true;
    }

#if DECODE_COOLIX48
    if (COOLIX48 == results.decode_type)
    {
        IRCoolixAC ac(kGpioUnused);
        ac.setRawFromCoolix48(results.value);
        *state = ac.toCommon(previous);
        state->protocol = COOLIX48;
        return true;
    }
#endif

    return false;
}

bool AirconIrService::send_current_state(char *error, size_t error_size)
{
    const stdAc::state_t *previous = previous_state_valid_ ? &previous_state_ : NULL;
    if (!ac_sender_.sendAc(current_state_, previous))
    {
        set_error(error, error_size, "send_failed");
        return false;
    }

    previous_state_ = current_state_;
    previous_state_valid_ = true;
    set_error(error, error_size, "");
    return true;
}

void AirconIrService::set_error(char *error, size_t error_size, const char *message) const
{
    if ((NULL == error) || (0U == error_size))
    {
        return;
    }

    snprintf(error, error_size, "%s", (NULL == message) ? "" : message);
}

void AirconIrService::update_field_strings(const char *json, stdAc::state_t *state) const
{
    char value[32];
    if (app_json_get_string(json, "mode", value, sizeof(value)))
    {
        state->mode = parse_mode(value, state->mode);
    }

    if (app_json_get_string(json, "fanspeed", value, sizeof(value)))
    {
        state->fanspeed = IRac::strToFanspeed(value, state->fanspeed);
    }

    if (app_json_get_string(json, "swingv", value, sizeof(value)))
    {
        state->swingv = IRac::strToSwingV(value, state->swingv);
    }

    if (app_json_get_string(json, "swingh", value, sizeof(value)))
    {
        state->swingh = IRac::strToSwingH(value, state->swingh);
    }
}

void AirconIrService::update_field_numbers(const char *json, stdAc::state_t *state) const
{
    int32_t int_value = 0;
    float float_value = 0.0f;

    if (app_json_get_int(json, "model", &int_value))
    {
        state->model = static_cast<int16_t>(int_value);
    }

    if (app_json_get_float(json, "degrees", &float_value))
    {
        state->degrees = float_value;
    }

    if (app_json_get_int(json, "sleep", &int_value))
    {
        state->sleep = static_cast<int16_t>(int_value);
    }

    if (app_json_get_int(json, "clock", &int_value))
    {
        state->clock = static_cast<int16_t>(int_value);
    }

    if (app_json_get_int(json, "command", &int_value))
    {
        if ((int_value >= static_cast<int32_t>(stdAc::ac_command_t::kControlCommand)) &&
            (int_value <= static_cast<int32_t>(stdAc::ac_command_t::kLastAcCommandEnum)))
        {
            state->command = static_cast<stdAc::ac_command_t>(int_value);
        }
    }

    if (app_json_get_float(json, "sensor_temperature", &float_value))
    {
        state->sensorTemperature = float_value;
    }
}

void AirconIrService::update_field_bools(const char *json, stdAc::state_t *state) const
{
    bool bool_value = false;

    if (app_json_get_bool(json, "power", &bool_value))
    {
        state->power = bool_value;
    }

    if (app_json_get_bool(json, "celsius", &bool_value))
    {
        state->celsius = bool_value;
    }

    if (app_json_get_bool(json, "quiet", &bool_value))
    {
        state->quiet = bool_value;
    }

    if (app_json_get_bool(json, "turbo", &bool_value))
    {
        state->turbo = bool_value;
    }

    if (app_json_get_bool(json, "econo", &bool_value))
    {
        state->econo = bool_value;
    }

    if (app_json_get_bool(json, "light", &bool_value))
    {
        state->light = bool_value;
    }

    if (app_json_get_bool(json, "filter", &bool_value))
    {
        state->filter = bool_value;
    }

    if (app_json_get_bool(json, "clean", &bool_value))
    {
        state->clean = bool_value;
    }

    if (app_json_get_bool(json, "beep", &bool_value))
    {
        state->beep = bool_value;
    }

    if (app_json_get_bool(json, "ifeel", &bool_value))
    {
        state->iFeel = bool_value;
    }
}
