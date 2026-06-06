#include "app/button_panel.h"

#include "Arduino.h"
#include "app/app_board_config.h"
#include "hal_data.h"

namespace
{
const uint32_t kDebounceMs = 30U;
}

ButtonPanel::ButtonPanel() :
    keys_{
        {APP_KEY_SELECT_PIN, false, false, 0U},
        {APP_KEY_CONFIRM_PIN, false, false, 0U},
        {APP_KEY_BACK_PIN, false, false, 0U},
    }
{
}

void ButtonPanel::begin()
{
    const uint32_t now = millis();
    for (size_t i = 0; i < 3U; i++)
    {
        keys_[i].raw_pressed = read_pressed(keys_[i].pin);
        keys_[i].stable_pressed = keys_[i].raw_pressed;
        keys_[i].last_change_ms = now;
    }
}

ButtonEvent ButtonPanel::poll()
{
    const uint32_t now = millis();
    if (poll_key(&keys_[0], now))
    {
        return ButtonEvent::Select;
    }
    if (poll_key(&keys_[1], now))
    {
        return ButtonEvent::Confirm;
    }
    if (poll_key(&keys_[2], now))
    {
        return ButtonEvent::Back;
    }

    return ButtonEvent::None;
}

bool ButtonPanel::read_pressed(bsp_io_port_pin_t pin) const
{
    bsp_io_level_t level = BSP_IO_LEVEL_HIGH;
    (void) g_ioport.p_api->pinRead(g_ioport.p_ctrl, pin, &level);
    return level == APP_KEY_ACTIVE_LEVEL;
}

bool ButtonPanel::poll_key(KeyState *key, uint32_t now_ms)
{
    if (NULL == key)
    {
        return false;
    }

    const bool raw = read_pressed(key->pin);
    if (raw != key->raw_pressed)
    {
        key->raw_pressed = raw;
        key->last_change_ms = now_ms;
        return false;
    }

    if ((now_ms - key->last_change_ms) < kDebounceMs)
    {
        return false;
    }

    if (raw != key->stable_pressed)
    {
        key->stable_pressed = raw;
        return raw;
    }

    return false;
}
