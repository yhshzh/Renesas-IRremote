#ifndef APP_BUTTON_PANEL_H_
#define APP_BUTTON_PANEL_H_

#include <stdint.h>

#include "bsp_api.h"

enum class ButtonEvent
{
    None,
    Select,
    Confirm,
    Back,
};

class ButtonPanel
{
public:
    ButtonPanel();

    void begin();
    ButtonEvent poll();

private:
    struct KeyState
    {
        bsp_io_port_pin_t pin;
        bool stable_pressed;
        bool raw_pressed;
        uint32_t last_change_ms;
    };

    bool read_pressed(bsp_io_port_pin_t pin) const;
    bool poll_key(KeyState *key, uint32_t now_ms);

    KeyState keys_[3];
};

#endif
