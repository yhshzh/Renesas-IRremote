#include "app/remote_controller.h"

#include <stdio.h>
#include <string.h>

#include "Arduino.h"
#include "app/app_json.h"

#define APP_STRINGIFY_HELPER(value) #value
#define APP_STRINGIFY(value) APP_STRINGIFY_HELPER(value)

#ifndef IRREMOTE_RA4M2_USE_IRQ_RECV
#define IRREMOTE_RA4M2_USE_IRQ_RECV 0
#endif

#ifndef IRREMOTE_RA4M2_USE_SYSTICK_RECV
#define IRREMOTE_RA4M2_USE_SYSTICK_RECV 0
#endif

#ifndef IRREMOTE_RA4M2_USE_GPT_SEND
#define IRREMOTE_RA4M2_USE_GPT_SEND 0
#endif

#if IRREMOTE_RA4M2_USE_IRQ_RECV
#define APP_IR_RECV_BACKEND "external_irq"
#elif IRREMOTE_RA4M2_USE_SYSTICK_RECV
#define APP_IR_RECV_BACKEND "systick"
#else
#define APP_IR_RECV_BACKEND "polling"
#endif

#if IRREMOTE_RA4M2_USE_GPT_SEND
#define APP_IR_SEND_BACKEND "gpt_pwm"
#else
#define APP_IR_SEND_BACKEND "gpio_software"
#endif

RemoteController::RemoteController() :
    transport_(),
    ir_(),
    ui_(),
    last_wifi_status_version_(0U)
{
}

void RemoteController::begin()
{
    ir_.begin();
    ui_.begin();
    transport_.begin();
    last_wifi_status_version_ = transport_.wifi_status_version();
}

void RemoteController::poll()
{
    TransportPacket packet;
    while (transport_.poll(&packet))
    {
        handle_packet(packet);
    }

    if (ir_.poll())
    {
        char payload[900];
        make_state_json(payload, sizeof(payload), "ir_rx");
        transport_.broadcast(payload);
    }

    const uint32_t wifi_status_version = transport_.wifi_status_version();
    if (wifi_status_version != last_wifi_status_version_)
    {
        last_wifi_status_version_ = wifi_status_version;
        broadcast_state("state");
    }

    handle_local_action(ui_.poll(ir_, transport_));
}

void RemoteController::handle_packet(const TransportPacket &packet)
{
    char kind[32];
    if (!app_json_get_string(packet.payload, "kind", kind, sizeof(kind)))
    {
        send_error(packet.link, "missing_kind");
        return;
    }

    if ((0 == strcmp(kind, "hello")) || (0 == strcmp(kind, "get_state")))
    {
        char payload[900];
        make_state_json(payload, sizeof(payload), "state");
        transport_.send(packet.link, payload);
    }
    else if (0 == strcmp(kind, "diag"))
    {
        char payload[512];
        make_diag_json(payload, sizeof(payload));
        transport_.send(packet.link, payload);
    }
    else if (0 == strcmp(kind, "learn_start"))
    {
        ir_.start_learning();
        send_ack(packet.link, "learn_started");
    }
    else if (0 == strcmp(kind, "learn_stop"))
    {
        ir_.stop_learning();
        send_ack(packet.link, "learn_stopped");
    }
    else if (0 == strcmp(kind, "ac_set"))
    {
        char error[48];
        if (!ir_.apply_state_json(packet.payload, error, sizeof(error)))
        {
            send_error(packet.link, error);
            return;
        }

        char payload[900];
        make_state_json(payload, sizeof(payload), "ir_tx");
        transport_.send(packet.link, payload);
    }
    else if (0 == strcmp(kind, "wifi_set"))
    {
        char ssid[80];
        char password[80];
        if (!app_json_get_string(packet.payload, "ssid", ssid, sizeof(ssid)) ||
            !app_json_get_string(packet.payload, "password", password, sizeof(password)))
        {
            send_error(packet.link, "missing_wifi_credentials");
            return;
        }

        send_ack(packet.link, "wifi_connecting");
        transport_.configure_wifi(ssid, password);
    }
    else if (0 == strcmp(kind, "wifi_reset"))
    {
        send_ack(packet.link, "wifi_resetting");
        transport_.reset_wifi();
        last_wifi_status_version_ = transport_.wifi_status_version();
        broadcast_state("state");
    }
    else if (0 == strcmp(kind, "ble_reset"))
    {
        send_ack(packet.link, "ble_resetting");
        transport_.reset_ble();
        broadcast_state("state");
    }
    else
    {
        send_error(packet.link, "unknown_kind");
    }
}

void RemoteController::handle_local_action(LocalUiAction action)
{
    switch (action)
    {
        case LocalUiAction::StartLearning:
            ir_.start_learning();
            ui_.notify("LEARN START");
            broadcast_ack("learn_started");
            break;
        case LocalUiAction::StopLearning:
            ir_.stop_learning();
            ui_.notify("LEARN STOP");
            broadcast_ack("learn_stopped");
            broadcast_state("state");
            break;
        case LocalUiAction::ResetBle:
            ui_.notify("RESET BLE");
            transport_.reset_ble();
            broadcast_state("state");
            break;
        case LocalUiAction::ResetWifi:
            ui_.notify("RESET WIFI");
            transport_.reset_wifi();
            last_wifi_status_version_ = transport_.wifi_status_version();
            broadcast_state("state");
            break;
        case LocalUiAction::BroadcastState:
            ui_.notify("SYNCED");
            broadcast_state("state");
            break;
        case LocalUiAction::None:
        default:
            break;
    }
}

void RemoteController::make_state_json(char *payload, size_t payload_size, const char *kind)
{
    if ((NULL == payload) || (0U == payload_size))
    {
        return;
    }

    ir_.state_json(payload, payload_size, kind);

    char wifi[96];
    transport_.wifi_json(wifi, sizeof(wifi));

    size_t len = strlen(payload);
    while ((len > 0U) && (('\n' == payload[len - 1U]) || ('\r' == payload[len - 1U])))
    {
        payload[len - 1U] = '\0';
        len--;
    }

    if ((0U == len) || ('}' != payload[len - 1U]))
    {
        return;
    }

    payload[len - 1U] = '\0';
    snprintf(payload + len - 1U,
             payload_size - (len - 1U),
             ",\"wifi\":%s}\n",
             wifi);
}

void RemoteController::make_diag_json(char *payload, size_t payload_size) const
{
    if ((NULL == payload) || (0U == payload_size))
    {
        return;
    }

    char wifi[96];
    char at[192];
    transport_.wifi_json(wifi, sizeof(wifi));
    transport_.at_diagnostics_json(at, sizeof(at));
    snprintf(payload,
             payload_size,
             "{\"kind\":\"diag\",\"protocol_version\":1,\"app_mode\":\"remote_controller\","
             "\"transport\":{\"uart_baud\":%u,\"tcp_port\":%u,"
             "\"ble_service_index\":%d,\"ble_write_char_index\":%d,"
             "\"ble_notify_char_index\":%d,"
             "\"ble_notify_chunk_size\":%u,\"at\":%s},"
             "\"ir\":{\"rx_pin\":\"%s\",\"tx_pin\":\"%s\","
             "\"recv_backend\":\"%s\",\"send_backend\":\"%s\","
             "\"recv_buffer_size\":%u,\"recv_timeout_ms\":%u},"
             "\"wifi\":%s}\n",
             static_cast<unsigned int>(APP_TRANSPORT_UART_BAUD),
             static_cast<unsigned int>(APP_TRANSPORT_TCP_PORT),
             transport_.ble_service_index(),
             transport_.ble_write_char_index(),
             transport_.ble_notify_char_index(),
             static_cast<unsigned int>(APP_BLE_NOTIFY_CHUNK_SIZE),
             at,
             APP_STRINGIFY(APP_IR_RX_PIN),
             APP_STRINGIFY(APP_IR_TX_PIN),
             APP_IR_RECV_BACKEND,
             APP_IR_SEND_BACKEND,
             static_cast<unsigned int>(APP_IR_RECV_BUFFER_SIZE),
             static_cast<unsigned int>(APP_IR_RECV_TIMEOUT_MS),
             wifi);
}

void RemoteController::broadcast_state(const char *kind)
{
    char payload[900];
    make_state_json(payload, sizeof(payload), kind);
    transport_.broadcast(payload);
}

void RemoteController::broadcast_ack(const char *kind)
{
    char payload[96];
    char escaped[48];
    app_json_escape(kind, escaped, sizeof(escaped));
    snprintf(payload, sizeof(payload), "{\"kind\":\"%s\",\"learning\":%s}\n", escaped, ir_.is_learning() ? "true" : "false");
    transport_.broadcast(payload);
}

void RemoteController::send_error(const TransportLink &link, const char *code)
{
    char payload[128];
    char escaped[64];
    app_json_escape(code, escaped, sizeof(escaped));
    snprintf(payload, sizeof(payload), "{\"kind\":\"error\",\"code\":\"%s\"}\n", escaped);
    transport_.send(link, payload);
}

void RemoteController::send_ack(const TransportLink &link, const char *kind)
{
    char payload[96];
    char escaped[48];
    app_json_escape(kind, escaped, sizeof(escaped));
    snprintf(payload, sizeof(payload), "{\"kind\":\"%s\",\"learning\":%s}\n", escaped, ir_.is_learning() ? "true" : "false");
    transport_.send(link, payload);
}
