#include "app/local_ui.h"

#include <stdio.h>
#include <string.h>

#include "Arduino.h"

namespace
{
const uint32_t kRenderPeriodMs = 120U;
const uint32_t kMessageMs = 1500U;

void bool_word(bool value, char *out, size_t out_size)
{
    snprintf(out, out_size, "%s", value ? "ON" : "OFF");
}

void ip_or_dash(const char *ip, char *out, size_t out_size)
{
    if ((NULL == ip) || ('\0' == ip[0]))
    {
        snprintf(out, out_size, "-");
    }
    else
    {
        snprintf(out, out_size, "%s", ip);
    }
}
}

LocalUi::LocalUi() :
    display_(),
    buttons_(),
    screen_(Screen::Home),
    menu_index_(0U),
    last_render_ms_(0U),
    message_until_ms_(0U),
    message_(),
    dirty_(true)
{
}

void LocalUi::begin()
{
    buttons_.begin();
    if (display_.begin())
    {
        display_.clear();
        display_.fill_rect(0, 0, 128, 12, true);
        display_.draw_text(4, 2, "RA4M2 IR", true);
        display_.draw_text(8, 24, "BOOTING...");
        display_.flush();
    }
    dirty_ = true;
}

LocalUiAction LocalUi::poll(const AirconIrService &ir, const EspAtTransport &transport)
{
    AirconStatus ir_status;
    TransportStatus transport_status;
    ir.status(&ir_status);
    transport.status(&transport_status);

    const ButtonEvent event = buttons_.poll();
    LocalUiAction action = handle_event(event, ir_status);
    if ((ButtonEvent::None != event) || (LocalUiAction::None != action))
    {
        dirty_ = true;
    }

    const uint32_t now = millis();
    if (dirty_ || ((now - last_render_ms_) >= kRenderPeriodMs))
    {
        render(ir_status, transport_status);
        last_render_ms_ = now;
        dirty_ = false;
    }

    return action;
}

void LocalUi::notify(const char *message)
{
    snprintf(message_, sizeof(message_), "%s", (NULL == message) ? "" : message);
    message_until_ms_ = millis() + kMessageMs;
    dirty_ = true;
}

LocalUiAction LocalUi::handle_event(ButtonEvent event, const AirconStatus &ir_status)
{
    if (ButtonEvent::None == event)
    {
        return LocalUiAction::None;
    }

    if (Screen::Home == screen_)
    {
        if (ButtonEvent::Select == event)
        {
            screen_ = Screen::Menu;
            return LocalUiAction::None;
        }
        if (ButtonEvent::Confirm == event)
        {
            return ir_status.learning ? LocalUiAction::StopLearning : LocalUiAction::StartLearning;
        }
        if (ButtonEvent::Back == event)
        {
            screen_ = Screen::Network;
            return LocalUiAction::None;
        }
    }
    else if (Screen::Menu == screen_)
    {
        if (ButtonEvent::Select == event)
        {
            menu_index_ = static_cast<uint8_t>((menu_index_ + 1U) % 5U);
        }
        else if (ButtonEvent::Back == event)
        {
            screen_ = Screen::Home;
        }
        else if (ButtonEvent::Confirm == event)
        {
            switch (menu_index_)
            {
                case 0:
                    return ir_status.learning ? LocalUiAction::StopLearning : LocalUiAction::StartLearning;
                case 1:
                    screen_ = Screen::Network;
                    break;
                case 2:
                    return LocalUiAction::ResetBle;
                case 3:
                    return LocalUiAction::ResetWifi;
                default:
                    screen_ = Screen::Home;
                    return LocalUiAction::BroadcastState;
            }
        }
    }
    else
    {
        if (ButtonEvent::Back == event)
        {
            screen_ = Screen::Menu;
        }
        else if (ButtonEvent::Select == event)
        {
            screen_ = Screen::Home;
        }
        else if (ButtonEvent::Confirm == event)
        {
            return LocalUiAction::BroadcastState;
        }
    }

    return LocalUiAction::None;
}

void LocalUi::render(const AirconStatus &ir_status, const TransportStatus &transport_status)
{
    if (!display_.is_ready())
    {
        return;
    }

    display_.clear();
    if (Screen::Home == screen_)
    {
        render_home(ir_status, transport_status);
    }
    else if (Screen::Menu == screen_)
    {
        render_menu(ir_status);
    }
    else
    {
        render_network(transport_status);
    }

    if ((message_[0] != '\0') && (static_cast<int32_t>(millis() - message_until_ms_) < 0))
    {
        display_.fill_rect(0, 54, 128, 10, true);
        display_.draw_text(4, 55, message_, true);
    }
    display_.flush();
}

void LocalUi::render_home(const AirconStatus &ir_status, const TransportStatus &transport_status)
{
    draw_header("RA4M2 IR", transport_status);

    if (ir_status.learning)
    {
        display_.draw_rect(4, 17, 120, 26, true);
        display_.draw_text(12, 22, "LEARNING IR");
        display_.draw_text(12, 34, "PRESS REMOTE");
    }
    else if (ir_status.valid)
    {
        char temp[20];
        snprintf(temp, sizeof(temp), "%dC", static_cast<int>(ir_status.degrees + 0.5f));
        display_.draw_text(8, 18, temp);
        display_.draw_text(46, 18, ir_status.power ? "ON" : "OFF");
        display_.draw_text(8, 30, ir_status.mode);
        display_.draw_text(58, 30, ir_status.fanspeed);
        display_.draw_text(8, 42, ir_status.protocol);
    }
    else
    {
        display_.draw_text(8, 22, "NO AC LEARNED");
        display_.draw_text(8, 36, "OK START LEARN");
    }

    draw_footer("B:NET S:MENU OK:LRN");
}

void LocalUi::render_menu(const AirconStatus &ir_status)
{
    TransportStatus blank_status;
    memset(&blank_status, 0, sizeof(blank_status));
    draw_header("MENU", blank_status);

    for (uint8_t i = 0U; i < 5U; i++)
    {
        const int y = 14 + (static_cast<int>(i) * 9);
        const bool selected = (i == menu_index_);
        if (selected)
        {
            display_.fill_rect(0, y - 1, 128, 9, true);
            display_.draw_text(2, y, ">", true);
        }
        display_.draw_text(12, y, menu_label(i, ir_status), selected);
    }

    draw_footer("S:NEXT OK:RUN B:HOME");
}

void LocalUi::render_network(const TransportStatus &transport_status)
{
    draw_header("NETWORK", transport_status);

    char value[24];
    bool_word(transport_status.ble_connected, value, sizeof(value));
    display_.draw_text(4, 15, "BLE");
    display_.draw_text(34, 15, value);
    bool_word(transport_status.tcp_connected, value, sizeof(value));
    display_.draw_text(74, 15, "TCP");
    display_.draw_text(104, 15, value);

    ip_or_dash(transport_status.ap_ip, value, sizeof(value));
    display_.draw_text(4, 29, "AP");
    display_.draw_text(24, 29, value);
    ip_or_dash(transport_status.station_ip, value, sizeof(value));
    display_.draw_text(4, 41, "STA");
    display_.draw_text(28, 41, value);

    draw_footer("S:HOME OK:SYNC B:MENU");
}

const char *LocalUi::menu_label(uint8_t index, const AirconStatus &ir_status) const
{
    switch (index)
    {
        case 0:
            return ir_status.learning ? "STOP LEARN" : "START LEARN";
        case 1:
            return "NETWORK";
        case 2:
            return "RESET BLE";
        case 3:
            return "RESET WIFI";
        default:
            return "HOME";
    }
}

void LocalUi::draw_header(const char *title, const TransportStatus &transport_status)
{
    display_.fill_rect(0, 0, 128, 11, true);
    display_.draw_text(3, 2, title, true);
    display_.draw_text_right(126, 2, transport_status.ble_connected ? "B" : "-", true);
    display_.draw_text_right(114, 2, transport_status.tcp_connected ? "T" : "-", true);
    display_.draw_text_right(102, 2, (transport_status.station_ip[0] != '\0') ? "W" : "-", true);
}

void LocalUi::draw_footer(const char *text)
{
    if ((message_[0] != '\0') && (static_cast<int32_t>(millis() - message_until_ms_) < 0))
    {
        return;
    }

    display_.draw_text(0, 56, text);
}
