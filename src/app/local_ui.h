#ifndef APP_LOCAL_UI_H_
#define APP_LOCAL_UI_H_

#include <stdint.h>

#include "app/aircon_ir_service.h"
#include "app/button_panel.h"
#include "app/esp_at_transport.h"
#include "app/oled_display.h"

enum class LocalUiAction
{
    None,
    StartLearning,
    StopLearning,
    ResetBle,
    ResetWifi,
    BroadcastState,
};

class LocalUi
{
public:
    LocalUi();

    void begin();
    LocalUiAction poll(const AirconIrService &ir, const EspAtTransport &transport);
    void notify(const char *message);

private:
    enum class Screen
    {
        Home,
        Menu,
        Network,
    };

    LocalUiAction handle_event(ButtonEvent event, const AirconStatus &ir_status);
    void render(const AirconStatus &ir_status, const TransportStatus &transport_status);
    void render_home(const AirconStatus &ir_status, const TransportStatus &transport_status);
    void render_menu(const AirconStatus &ir_status);
    void render_network(const TransportStatus &transport_status);
    const char *menu_label(uint8_t index, const AirconStatus &ir_status) const;
    void draw_header(const char *title, const TransportStatus &transport_status);
    void draw_footer(const char *text);

    OledDisplay display_;
    ButtonPanel buttons_;
    Screen screen_;
    uint8_t menu_index_;
    uint32_t last_render_ms_;
    uint32_t message_until_ms_;
    char message_[24];
    bool dirty_;
};

#endif
