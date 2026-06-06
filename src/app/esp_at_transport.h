#ifndef APP_ESP_AT_TRANSPORT_H_
#define APP_ESP_AT_TRANSPORT_H_

#include <stddef.h>
#include <stdint.h>

#include "app/app_board_config.h"
#include "app/ra_sci9_uart.h"

enum class TransportKind
{
    Tcp,
    Ble,
};

struct TransportLink
{
    TransportKind kind;
    int id;
};

struct TransportPacket
{
    TransportLink link;
    char payload[768];
};

struct TransportStatus
{
    bool tcp_connected;
    bool ble_connected;
    char ap_ip[24];
    char station_ip[24];
    uint32_t at_error_count;
};

class EspAtTransport
{
public:
    EspAtTransport();

    void begin();
    bool poll(TransportPacket *packet);
    void send(const TransportLink &link, const char *payload);
    void broadcast(const char *payload);
    void configure_wifi(const char *ssid, const char *password);
    void reset_wifi();
    void reset_ble();
    void wifi_json(char *out, size_t out_size) const;
    void at_diagnostics_json(char *out, size_t out_size) const;
    void status(TransportStatus *out) const;
    uint32_t wifi_status_version() const;
    int ble_service_index() const;
    int ble_write_char_index() const;
    int ble_notify_char_index() const;

private:
    void send_at(const char *command);
    void send_at_wait(const char *command, uint32_t timeout_ms, bool record_error = true);
    bool wait_for_at_result(uint32_t timeout_ms);
    bool wait_for_prompt(uint32_t timeout_ms);
    void send_data_command(const char *command, const char *payload);
    void send_ble_data(int conn, const char *payload);
    bool process_line(const char *line, TransportPacket *packet);
    bool parse_ipd(const char *line, TransportPacket *packet);
    bool parse_ble_write(const char *line, TransportPacket *packet);
    bool parse_payload_continuation(const char *line, TransportPacket *packet);
    bool accumulate_packet(const TransportLink &link, const char *payload, int declared_len, TransportPacket *packet);
    void process_gatts_char_line(const char *line);
    void process_tcp_link_event(const char *line);
    void process_wifi_line(const char *line);
    void record_at_error(const char *command);
    void set_wifi_ip(char *target, size_t target_size, const char *value);
    void schedule_wifi_info_query(uint32_t delay_ms, uint8_t attempts);
    void service_wifi_info_query();
    void append_rx(uint8_t value);
    void drain_rx(uint32_t timeout_ms);

    RaSci9Uart uart_;
    char line_[900];
    size_t line_len_;
    TransportLink pending_link_;
    char pending_payload_[sizeof(TransportPacket::payload)];
    size_t pending_payload_len_;
    bool pending_payload_active_;
    TransportLink continuation_link_;
    size_t continuation_remaining_len_;
    bool continuation_active_;
    int last_tcp_link_;
    bool tcp_link_active_;
    int last_ble_conn_;
    int ble_service_index_;
    int ble_write_char_index_;
    int ble_notify_char_index_;
    char ap_ip_[24];
    char station_ip_[24];
    uint32_t at_error_count_;
    char last_at_error_command_[96];
    char last_at_result_[24];
    uint32_t next_wifi_info_query_ms_;
    uint8_t wifi_info_query_attempts_;
    uint32_t wifi_status_version_;
};

#endif
