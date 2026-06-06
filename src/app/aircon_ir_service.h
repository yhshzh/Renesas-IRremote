#ifndef APP_AIRCON_IR_SERVICE_H_
#define APP_AIRCON_IR_SERVICE_H_

#include <stddef.h>
#include <stdint.h>

#include "IRac.h"
#include "IRrecv.h"
#include "app/app_board_config.h"

struct IrFrameInfo
{
    decode_type_t decode_type;
    uint64_t value;
    uint16_t bits;
    uint16_t rawlen;
    bool overflow;
};

struct AirconStatus
{
    bool valid;
    bool learning;
    char protocol[24];
    bool power;
    char mode[12];
    float degrees;
    char fanspeed[16];
    char frame_protocol[24];
    uint16_t frame_bits;
    uint16_t frame_rawlen;
    bool frame_overflow;
};

class AirconIrService
{
public:
    AirconIrService();

    void begin();
    void start_learning();
    void stop_learning();
    bool is_learning() const;
    bool poll();
    bool apply_state_json(const char *json, char *error, size_t error_size);
    void state_json(char *out, size_t out_size, const char *kind) const;
    void status(AirconStatus *out) const;

private:
    bool decode_ac_state(const decode_results &results, stdAc::state_t *state);
    bool send_current_state(char *error, size_t error_size);
    void set_error(char *error, size_t error_size, const char *message) const;
    void update_field_strings(const char *json, stdAc::state_t *state) const;
    void update_field_numbers(const char *json, stdAc::state_t *state) const;
    void update_field_bools(const char *json, stdAc::state_t *state) const;

    IRrecv receiver_;
    IRac ac_sender_;
    decode_results results_;
    stdAc::state_t current_state_;
    stdAc::state_t previous_state_;
    IrFrameInfo last_frame_;
    bool learning_;
    bool receiver_enabled_;
    bool current_state_valid_;
    bool previous_state_valid_;
};

#endif
