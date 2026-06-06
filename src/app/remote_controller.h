#ifndef APP_REMOTE_CONTROLLER_H_
#define APP_REMOTE_CONTROLLER_H_

#include <stddef.h>

#include "app/aircon_ir_service.h"
#include "app/esp_at_transport.h"
#include "app/local_ui.h"

class RemoteController
{
public:
    RemoteController();

    void begin();
    void poll();

private:
    void handle_packet(const TransportPacket &packet);
    void handle_local_action(LocalUiAction action);
    void make_state_json(char *payload, size_t payload_size, const char *kind);
    void make_diag_json(char *payload, size_t payload_size) const;
    void broadcast_state(const char *kind);
    void broadcast_ack(const char *kind);
    void send_error(const TransportLink &link, const char *code);
    void send_ack(const TransportLink &link, const char *kind);

    EspAtTransport transport_;
    AirconIrService ir_;
    LocalUi ui_;
    uint32_t last_wifi_status_version_;
};

#endif
