#include "app/esp_at_transport.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Arduino.h"
#include "app/app_json.h"

namespace
{
const char *kDeviceName = "RA4M2_IR";
const char *kSoftApPassword = "12345678";

void copy_text(char *out, size_t out_size, const char *value)
{
    if ((NULL == out) || (0U == out_size))
    {
        return;
    }

    snprintf(out, out_size, "%s", (NULL == value) ? "" : value);
}

void copy_payload(char *out, size_t out_size, const char *payload, int declared_len)
{
    if ((NULL == out) || (0U == out_size))
    {
        return;
    }

    if (NULL == payload)
    {
        out[0] = '\0';
        return;
    }

    size_t payload_len = strlen(payload);
    if ((declared_len >= 0) && (static_cast<size_t>(declared_len) < payload_len))
    {
        payload_len = static_cast<size_t>(declared_len);
    }

    const size_t copy_len = (payload_len < (out_size - 1U)) ? payload_len : (out_size - 1U);
    memcpy(out, payload, copy_len);
    out[copy_len] = '\0';
}

bool copy_quoted_field(const char *line, char *out, size_t out_size)
{
    const char *start = strchr(line, '"');
    if (NULL == start)
    {
        return false;
    }

    start++;
    const char *end = strchr(start, '"');
    if ((NULL == end) || (end < start))
    {
        return false;
    }

    const size_t field_len = static_cast<size_t>(end - start);
    const size_t copy_len = (field_len < (out_size - 1U)) ? field_len : (out_size - 1U);
    memcpy(out, start, copy_len);
    out[copy_len] = '\0';
    return true;
}

}

EspAtTransport::EspAtTransport() :
    uart_(),
    line_(),
    line_len_(0U),
    pending_link_{TransportKind::Tcp, 0},
    pending_payload_(),
    pending_payload_len_(0U),
    pending_payload_active_(false),
    continuation_link_{TransportKind::Tcp, 0},
    continuation_remaining_len_(0U),
    continuation_active_(false),
    last_tcp_link_(0),
    tcp_link_active_(false),
    last_ble_conn_(-1),
    ble_service_index_(APP_BLE_GATT_SERVICE_INDEX),
    ble_write_char_index_(APP_BLE_GATT_WRITE_CHAR_INDEX),
    ble_notify_char_index_(APP_BLE_GATT_NOTIFY_CHAR_INDEX),
    ap_ip_(),
    station_ip_(),
    at_error_count_(0U),
    last_at_error_command_(),
    last_at_result_(),
    next_wifi_info_query_ms_(0U),
    wifi_info_query_attempts_(0U),
    wifi_status_version_(0U)
{
}

void EspAtTransport::begin()
{
    uart_.begin(APP_TRANSPORT_UART_BAUD);
    R_BSP_SoftwareDelay(500U, BSP_DELAY_UNITS_MILLISECONDS);
    drain_rx(100U);

    send_at_wait("AT", 1000U);
    drain_rx(100U);
    send_at_wait("ATE0", 1000U);
    send_at_wait("AT+CWMODE=3", 1000U);

    char command[96];
    snprintf(command,
             sizeof(command),
             "AT+CWSAP=\"%s\",\"%s\",5,3",
             kDeviceName,
             kSoftApPassword);
    send_at_wait(command, 2000U);

    send_at_wait("AT+CIPMUX=1", 1000U);
    snprintf(command, sizeof(command), "AT+CIPSERVER=1,%u", static_cast<unsigned int>(APP_TRANSPORT_TCP_PORT));
    send_at_wait(command, 1000U);
    schedule_wifi_info_query(100U, 2U);

    send_at_wait("AT+BLEINIT=0", 1000U, false);
    drain_rx(100U);
    send_at_wait("AT+BLEINIT=2", 2000U);
    snprintf(command, sizeof(command), "AT+BLENAME=\"%s\"", kDeviceName);
    send_at_wait(command, 1000U);
    send_at_wait("AT+BLEGATTSSRVCRE", 3000U);
    send_at_wait("AT+BLEGATTSSRVSTART", 2000U);
    send_at_wait("AT+BLEGATTSCHAR?", 2000U, false);
    send_at_wait("AT+BLEADVSTOP", 1000U, false);
    send_at_wait("AT+BLEADVDATAEX=\"RA4M2_IR\",\"A002\",\"5241344D325F4952\",1", 1000U);
    send_at_wait("AT+BLEADVSTART", 1000U);
}

bool EspAtTransport::poll(TransportPacket *packet)
{
    service_wifi_info_query();

    uint8_t value = 0U;
    while (uart_.read(&value))
    {
        if ('\r' == value)
        {
            continue;
        }

        if ('\n' == value)
        {
            if (line_len_ > 0U)
            {
                line_[line_len_] = '\0';
                line_len_ = 0U;
                if (process_line(line_, packet))
                {
                    return true;
                }
                service_wifi_info_query();
            }
            continue;
        }

        append_rx(value);
    }

    return false;
}

void EspAtTransport::send(const TransportLink &link, const char *payload)
{
    if (NULL == payload)
    {
        return;
    }

    char command[64];
    char framed_payload[1024];
    const size_t payload_len = strlen(payload);
    if (payload_len < (sizeof(framed_payload) - 2U))
    {
        memcpy(framed_payload, payload, payload_len);
        framed_payload[payload_len] = '\n';
        framed_payload[payload_len + 1U] = '\0';
        payload = framed_payload;
    }

    const unsigned int length = static_cast<unsigned int>(strlen(payload));

    if (TransportKind::Tcp == link.kind)
    {
        snprintf(command, sizeof(command), "AT+CIPSEND=%d,%u", link.id, length);
        send_data_command(command, payload);
        last_tcp_link_ = link.id;
    }
    else
    {
        const int conn = (link.id >= 0) ? link.id : last_ble_conn_;
        if (conn < 0)
        {
            return;
        }

        send_ble_data(conn, payload);
        last_ble_conn_ = conn;
    }
}

void EspAtTransport::broadcast(const char *payload)
{
    if (tcp_link_active_)
    {
        TransportLink tcp = {TransportKind::Tcp, last_tcp_link_};
        send(tcp, payload);
    }

    if (last_ble_conn_ >= 0)
    {
        TransportLink ble = {TransportKind::Ble, last_ble_conn_};
        send(ble, payload);
    }
}

void EspAtTransport::configure_wifi(const char *ssid, const char *password)
{
    char ssid_escaped[80];
    char password_escaped[80];
    char command[192];

    app_json_escape(ssid, ssid_escaped, sizeof(ssid_escaped));
    app_json_escape(password, password_escaped, sizeof(password_escaped));
    snprintf(command,
             sizeof(command),
             "AT+CWJAP=\"%s\",\"%s\"",
             ssid_escaped,
             password_escaped);
    set_wifi_ip(station_ip_, sizeof(station_ip_), "");
    send_at(command);
    schedule_wifi_info_query(2000U, 12U);
}

void EspAtTransport::reset_wifi()
{
    send_at_wait("AT+CWQAP", 1000U, false);
    set_wifi_ip(station_ip_, sizeof(station_ip_), "");
    send_at_wait("AT+CIPSERVER=0", 1000U, false);

    char command[48];
    snprintf(command, sizeof(command), "AT+CIPSERVER=1,%u", static_cast<unsigned int>(APP_TRANSPORT_TCP_PORT));
    send_at_wait(command, 1000U);
    tcp_link_active_ = false;
    schedule_wifi_info_query(200U, 3U);
}

void EspAtTransport::reset_ble()
{
    if (last_ble_conn_ >= 0)
    {
        char command[40];
        snprintf(command, sizeof(command), "AT+BLEDISCONN=%d", last_ble_conn_);
        send_at_wait(command, 1000U, false);
    }

    send_at_wait("AT+BLEADVSTOP", 1000U, false);
    send_at_wait("AT+BLEADVSTART", 1000U);
    last_ble_conn_ = -1;
}

void EspAtTransport::wifi_json(char *out, size_t out_size) const
{
    if ((NULL == out) || (0U == out_size))
    {
        return;
    }

    char ap_ip_escaped[32];
    char station_ip_escaped[32];
    app_json_escape(ap_ip_, ap_ip_escaped, sizeof(ap_ip_escaped));
    app_json_escape(station_ip_, station_ip_escaped, sizeof(station_ip_escaped));
    snprintf(out,
             out_size,
             "{\"ap_ip\":\"%s\",\"station_ip\":\"%s\"}",
             ap_ip_escaped,
             station_ip_escaped);
}

void EspAtTransport::status(TransportStatus *out) const
{
    if (NULL == out)
    {
        return;
    }

    memset(out, 0, sizeof(*out));
    out->tcp_connected = tcp_link_active_;
    out->ble_connected = (last_ble_conn_ >= 0);
    copy_text(out->ap_ip, sizeof(out->ap_ip), ap_ip_);
    copy_text(out->station_ip, sizeof(out->station_ip), station_ip_);
    out->at_error_count = at_error_count_;
}

void EspAtTransport::at_diagnostics_json(char *out, size_t out_size) const
{
    if ((NULL == out) || (0U == out_size))
    {
        return;
    }

    char command_escaped[128];
    char result_escaped[32];
    app_json_escape(last_at_error_command_, command_escaped, sizeof(command_escaped));
    app_json_escape(last_at_result_, result_escaped, sizeof(result_escaped));
    snprintf(out,
             out_size,
             "{\"error_count\":%lu,\"last_error_command\":\"%s\",\"last_result\":\"%s\"}",
             static_cast<unsigned long>(at_error_count_),
             command_escaped,
             result_escaped);
}

uint32_t EspAtTransport::wifi_status_version() const
{
    return wifi_status_version_;
}

int EspAtTransport::ble_service_index() const
{
    return ble_service_index_;
}

int EspAtTransport::ble_write_char_index() const
{
    return ble_write_char_index_;
}

int EspAtTransport::ble_notify_char_index() const
{
    return ble_notify_char_index_;
}

void EspAtTransport::send_at(const char *command)
{
    if (NULL == command)
    {
        return;
    }

    uart_.write(command);
    uart_.write("\r\n");
    uart_.flush_tx();
    R_BSP_SoftwareDelay(30U, BSP_DELAY_UNITS_MILLISECONDS);
}

void EspAtTransport::send_at_wait(const char *command, uint32_t timeout_ms, bool record_error)
{
    if (NULL == command)
    {
        return;
    }

    uart_.write(command);
    uart_.write("\r\n");
    uart_.flush_tx();
    if (!wait_for_at_result(timeout_ms) && record_error)
    {
        record_at_error(command);
    }
}

bool EspAtTransport::wait_for_at_result(uint32_t timeout_ms)
{
    const uint32_t start = millis();
    char response[128];
    size_t response_len = 0U;
    uint8_t value = 0U;

    while ((millis() - start) < timeout_ms)
    {
        if (!uart_.read(&value))
        {
            continue;
        }

        if ('\r' == value)
        {
            continue;
        }

        if ('\n' == value)
        {
            if (0U == response_len)
            {
                continue;
            }

            response[response_len] = '\0';
            if ((0 == strcmp(response, "OK")) ||
                (0 == strcmp(response, "SEND OK")) ||
                (0 == strcmp(response, "ready")))
            {
                copy_text(last_at_result_, sizeof(last_at_result_), response);
                return true;
            }

            if ((0 == strcmp(response, "ERROR")) ||
                (0 == strcmp(response, "FAIL")) ||
                (0 == strcmp(response, "SEND FAIL")) ||
                (0 == strncmp(response, "busy", 4U)))
            {
                copy_text(last_at_result_, sizeof(last_at_result_), response);
                return false;
            }

            process_gatts_char_line(response);
            response_len = 0U;
            continue;
        }

        if (response_len < (sizeof(response) - 1U))
        {
            response[response_len++] = static_cast<char>(value);
        }
        else
        {
            response_len = 0U;
        }
    }

    copy_text(last_at_result_, sizeof(last_at_result_), "timeout");
    return false;
}

bool EspAtTransport::wait_for_prompt(uint32_t timeout_ms)
{
    const uint32_t start = millis();
    uint8_t value = 0U;

    while ((millis() - start) < timeout_ms)
    {
        if (uart_.read(&value))
        {
            if ('>' == value)
            {
                return true;
            }

            if (('\r' != value) && ('\n' != value))
            {
                append_rx(value);
            }
            else if (('\n' == value) && (line_len_ > 0U))
            {
                line_[line_len_] = '\0';
                line_len_ = 0U;
            }
        }
    }

    return false;
}

void EspAtTransport::send_data_command(const char *command, const char *payload)
{
    if ((NULL == command) || (NULL == payload))
    {
        return;
    }

    uart_.write(command);
    uart_.write("\r\n");
    uart_.flush_tx();

    if (wait_for_prompt(700U))
    {
        uart_.write(reinterpret_cast<const uint8_t *>(payload), strlen(payload));
        uart_.flush_tx();
        R_BSP_SoftwareDelay(20U, BSP_DELAY_UNITS_MILLISECONDS);
    }
    else
    {
        copy_text(last_at_result_, sizeof(last_at_result_), "no_prompt");
        record_at_error(command);
    }
}

void EspAtTransport::send_ble_data(int conn, const char *payload)
{
    if ((conn < 0) || (NULL == payload))
    {
        return;
    }

    const size_t total_len = strlen(payload);
    size_t offset = 0U;
    while (offset < total_len)
    {
        size_t chunk_len = total_len - offset;
        if (chunk_len > APP_BLE_NOTIFY_CHUNK_SIZE)
        {
            chunk_len = APP_BLE_NOTIFY_CHUNK_SIZE;
        }

        char chunk[APP_BLE_NOTIFY_CHUNK_SIZE + 1U];
        memcpy(chunk, payload + offset, chunk_len);
        chunk[chunk_len] = '\0';

        char command[64];
        snprintf(command,
                 sizeof(command),
                 "AT+BLEGATTSNTFY=%d,%d,%d,%u",
                 conn,
                 ble_service_index_,
                 ble_notify_char_index_,
                 static_cast<unsigned int>(chunk_len));
        send_data_command(command, chunk);
        offset += chunk_len;
    }
}

bool EspAtTransport::process_line(const char *line, TransportPacket *packet)
{
    if ((NULL == line) || ('\0' == line[0]))
    {
        return false;
    }

    if (parse_payload_continuation(line, packet))
    {
        return true;
    }

    if (0 == strncmp(line, "+IPD,", 5U))
    {
        return parse_ipd(line, packet);
    }

    if (0 == strncmp(line, "+WRITE:", 7U))
    {
        return parse_ble_write(line, packet);
    }

    if (0 == strncmp(line, "+BLECONN:", 9U))
    {
        last_ble_conn_ = atoi(line + 9);
    }
    else if (0 == strncmp(line, "+BLEDISCONN:", 12U))
    {
        last_ble_conn_ = -1;
    }
    else
    {
        process_gatts_char_line(line);
        process_tcp_link_event(line);
        process_wifi_line(line);
    }

    return false;
}

bool EspAtTransport::parse_ipd(const char *line, TransportPacket *packet)
{
    if (NULL == packet)
    {
        return false;
    }

    const char *id_start = line + 5;
    char *after_id = NULL;
    const long link_id = strtol(id_start, &after_id, 10);
    if ((after_id == id_start) || (',' != *after_id))
    {
        return false;
    }

    char *after_len = NULL;
    const long declared_len = strtol(after_id + 1, &after_len, 10);
    if ((after_len == (after_id + 1)) || (':' != *after_len))
    {
        return false;
    }

    packet->link.kind = TransportKind::Tcp;
    packet->link.id = static_cast<int>(link_id);
    last_tcp_link_ = packet->link.id;
    tcp_link_active_ = true;
    return accumulate_packet(packet->link, after_len + 1, static_cast<int>(declared_len), packet);
}

bool EspAtTransport::parse_ble_write(const char *line, TransportPacket *packet)
{
    if (NULL == packet)
    {
        return false;
    }

    const char *conn_start = line + 7;
    char *after_conn = NULL;
    const long conn = strtol(conn_start, &after_conn, 10);
    if ((after_conn == conn_start) || (',' != *after_conn))
    {
        return false;
    }

    char *after_service = NULL;
    const long service_index = strtol(after_conn + 1, &after_service, 10);
    if ((after_service == (after_conn + 1)) || (',' != *after_service))
    {
        return false;
    }

    char *after_char = NULL;
    const long char_index = strtol(after_service + 1, &after_char, 10);
    if ((after_char == (after_service + 1)) || (',' != *after_char))
    {
        return false;
    }

    last_ble_conn_ = static_cast<int>(conn);

    if (static_cast<int>(service_index) != ble_service_index_)
    {
        return false;
    }

    if (char_index < 0)
    {
        return false;
    }

    if (static_cast<int>(char_index) != ble_write_char_index_)
    {
        return false;
    }

    const char *tail = after_char + 1;
    const char *first_comma = strchr(tail, ',');
    if (NULL == first_comma)
    {
        return false;
    }

    const char *len_start = tail;
    const char *payload_comma = first_comma;
    if (first_comma == tail)
    {
        len_start = first_comma + 1;
        payload_comma = strchr(len_start, ',');
        if (NULL == payload_comma)
        {
            return false;
        }
    }
    else
    {
        const char *second_comma = strchr(first_comma + 1, ',');
        if (NULL != second_comma)
        {
            char *after_second_number = NULL;
            strtol(first_comma + 1, &after_second_number, 10);
            if ((after_second_number != (first_comma + 1)) && (after_second_number == second_comma))
            {
                len_start = first_comma + 1;
                payload_comma = second_comma;
            }
        }
    }

    char *after_len = NULL;
    const long declared_len = strtol(len_start, &after_len, 10);
    if ((after_len == len_start) || (after_len != payload_comma))
    {
        return false;
    }

    packet->link.kind = TransportKind::Ble;
    packet->link.id = static_cast<int>(conn);
    return accumulate_packet(packet->link, payload_comma + 1, static_cast<int>(declared_len), packet);
}

bool EspAtTransport::parse_payload_continuation(const char *line, TransportPacket *packet)
{
    if (!continuation_active_ || (NULL == line) || (NULL == packet))
    {
        return false;
    }

    if ('{' != line[0])
    {
        continuation_remaining_len_ = 0U;
        continuation_active_ = false;
        return false;
    }

    const size_t line_len = strlen(line);
    packet->link = continuation_link_;
    copy_payload(packet->payload, sizeof(packet->payload), line, static_cast<int>(line_len));

    const size_t consumed_len = line_len + 1U;
    if (consumed_len >= continuation_remaining_len_)
    {
        continuation_remaining_len_ = 0U;
        continuation_active_ = false;
    }
    else
    {
        continuation_remaining_len_ -= consumed_len;
    }

    return true;
}

bool EspAtTransport::accumulate_packet(const TransportLink &link,
                                       const char *payload,
                                       int declared_len,
                                       TransportPacket *packet)
{
    if ((NULL == payload) || (NULL == packet))
    {
        return false;
    }

    const size_t raw_payload_len = strlen(payload);
    size_t payload_len = raw_payload_len;
    if ((declared_len >= 0) && (static_cast<size_t>(declared_len) < payload_len))
    {
        payload_len = static_cast<size_t>(declared_len);
    }

    const char *newline = static_cast<const char *>(memchr(payload, '\n', payload_len));
    bool end_of_packet = (NULL != newline);
    if (end_of_packet)
    {
        payload_len = static_cast<size_t>(newline - payload);
    }
    else if ((declared_len >= 0) && (payload_len < static_cast<size_t>(declared_len)))
    {
        // The UART line parser consumes '\n', so a short line means the command
        // delimiter was present in the ESP-AT payload.
        end_of_packet = true;
    }

    continuation_active_ = false;
    continuation_remaining_len_ = 0U;
    if (end_of_packet && (declared_len >= 0))
    {
        const size_t declared_size = static_cast<size_t>(declared_len);
        if (declared_size > (raw_payload_len + 1U))
        {
            continuation_link_ = link;
            continuation_remaining_len_ = declared_size - raw_payload_len - 1U;
            continuation_active_ = true;
        }
    }

    if (!pending_payload_active_ ||
        (pending_link_.kind != link.kind) ||
        (pending_link_.id != link.id))
    {
        pending_link_ = link;
        pending_payload_len_ = 0U;
        pending_payload_active_ = true;
    }

    if ((payload_len + pending_payload_len_) >= sizeof(pending_payload_))
    {
        pending_payload_len_ = 0U;
        pending_payload_active_ = false;
        return false;
    }

    memcpy(pending_payload_ + pending_payload_len_, payload, payload_len);
    pending_payload_len_ += payload_len;
    pending_payload_[pending_payload_len_] = '\0';

    if (!end_of_packet)
    {
        return false;
    }

    packet->link = link;
    copy_payload(packet->payload,
                 sizeof(packet->payload),
                 pending_payload_,
                 static_cast<int>(pending_payload_len_));
    pending_payload_len_ = 0U;
    pending_payload_active_ = false;
    return true;
}

void EspAtTransport::process_gatts_char_line(const char *line)
{
    const char *prefix = "+BLEGATTSCHAR:\"char\",";
    const size_t prefix_len = strlen(prefix);
    if ((NULL == line) || (0 != strncmp(line, prefix, prefix_len)))
    {
        return;
    }

    const char *service_start = line + prefix_len;
    char *after_service = NULL;
    const long service_index = strtol(service_start, &after_service, 10);
    if ((after_service == service_start) || (',' != *after_service))
    {
        return;
    }

    char *after_char = NULL;
    const long char_index = strtol(after_service + 1, &after_char, 10);
    if ((after_char == (after_service + 1)) || (',' != *after_char))
    {
        return;
    }

    const char *uuid_start = after_char + 1;
    char *after_uuid = NULL;
    unsigned long uuid = strtoul(uuid_start, &after_uuid, 0);
    if (after_uuid == uuid_start)
    {
        uuid = strtoul(uuid_start, &after_uuid, 16);
    }

    if ((after_uuid == uuid_start) || (service_index < 0) || (char_index < 0))
    {
        return;
    }

    if (0xC304UL == uuid)
    {
        ble_service_index_ = static_cast<int>(service_index);
        ble_write_char_index_ = static_cast<int>(char_index);
    }
    else if (0xC305UL == uuid)
    {
        ble_service_index_ = static_cast<int>(service_index);
        ble_notify_char_index_ = static_cast<int>(char_index);
    }
}

void EspAtTransport::process_tcp_link_event(const char *line)
{
    if (NULL == line)
    {
        return;
    }

    char *after_id = NULL;
    const long link_id = strtol(line, &after_id, 10);
    if ((after_id == line) || (',' != *after_id))
    {
        return;
    }

    if (0 == strcmp(after_id, ",CONNECT"))
    {
        last_tcp_link_ = static_cast<int>(link_id);
        tcp_link_active_ = true;
    }
    else if (0 == strcmp(after_id, ",CLOSED"))
    {
        if (static_cast<int>(link_id) == last_tcp_link_)
        {
            tcp_link_active_ = false;
        }
    }
}

void EspAtTransport::process_wifi_line(const char *line)
{
    if (NULL == line)
    {
        return;
    }

    if (0 == strncmp(line, "+CIFSR:APIP,", 12U))
    {
        char ip[24];
        if (copy_quoted_field(line + 12, ip, sizeof(ip)))
        {
            set_wifi_ip(ap_ip_, sizeof(ap_ip_), ip);
        }
    }
    else if (0 == strncmp(line, "+CIFSR:STAIP,", 13U))
    {
        char ip[24];
        if (copy_quoted_field(line + 13, ip, sizeof(ip)))
        {
            set_wifi_ip(station_ip_, sizeof(station_ip_), ip);
        }
    }
    else if (0 == strcmp(line, "WIFI GOT IP"))
    {
        schedule_wifi_info_query(100U, 4U);
    }
    else if ((0 == strcmp(line, "WIFI DISCONNECT")) || (0 == strcmp(line, "WIFI DISCONNECTED")))
    {
        set_wifi_ip(station_ip_, sizeof(station_ip_), "");
    }
}

void EspAtTransport::record_at_error(const char *command)
{
    at_error_count_++;
    copy_text(last_at_error_command_, sizeof(last_at_error_command_), command);
}

void EspAtTransport::set_wifi_ip(char *target, size_t target_size, const char *value)
{
    if ((NULL == target) || (0U == target_size))
    {
        return;
    }

    const char *next = (NULL == value) ? "" : value;
    if (0 != strncmp(target, next, target_size))
    {
        copy_text(target, target_size, next);
        wifi_status_version_++;
    }
}

void EspAtTransport::schedule_wifi_info_query(uint32_t delay_ms, uint8_t attempts)
{
    next_wifi_info_query_ms_ = millis() + delay_ms;
    wifi_info_query_attempts_ = attempts;
}

void EspAtTransport::service_wifi_info_query()
{
    if (0U == wifi_info_query_attempts_)
    {
        return;
    }

    const uint32_t now = millis();
    if (static_cast<int32_t>(now - next_wifi_info_query_ms_) < 0)
    {
        return;
    }

    send_at("AT+CIFSR");
    wifi_info_query_attempts_--;
    next_wifi_info_query_ms_ = now + 2000U;
}

void EspAtTransport::append_rx(uint8_t value)
{
    if (line_len_ < (sizeof(line_) - 1U))
    {
        line_[line_len_] = static_cast<char>(value);
        line_len_++;
    }
    else
    {
        line_len_ = 0U;
    }
}

void EspAtTransport::drain_rx(uint32_t timeout_ms)
{
    const uint32_t start = millis();
    uint8_t value = 0U;

    while ((millis() - start) < timeout_ms)
    {
        if (uart_.read(&value))
        {
            continue;
        }
    }
}
