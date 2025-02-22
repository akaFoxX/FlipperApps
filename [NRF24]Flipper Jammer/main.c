#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <nrf24.h>
#include <stdlib.h>
#include "jammer.h"

#define POWER_LEVELS 4
static uint8_t power_levels[POWER_LEVELS] = {NRF24_PA_MIN, NRF24_PA_LOW, NRF24_PA_HIGH, NRF24_PA_MAX};
static uint8_t current_power_index = 3;

static uint8_t start_channel = 2;
static uint8_t end_channel = 80;
static uint16_t attack_duration = 5000; // Tempo do jammer em ms
static uint8_t target_channel = 2; // Canal alvo identificado

void update_screen(Canvas* canvas) {
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 10, 10, "Flipper Zero Jammer");

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Start Channel: %d", start_channel);
    canvas_draw_str(canvas, 10, 30, buffer);
    snprintf(buffer, sizeof(buffer), "End Channel: %d", end_channel);
    canvas_draw_str(canvas, 10, 50, buffer);
    snprintf(buffer, sizeof(buffer), "Power Level: %d", current_power_index);
    canvas_draw_str(canvas, 10, 70, buffer);
    snprintf(buffer, sizeof(buffer), "Attack Time: %ds", attack_duration / 1000);
    canvas_draw_str(canvas, 10, 90, buffer);
    snprintf(buffer, sizeof(buffer), "Target Channel: %d", target_channel);
    canvas_draw_str(canvas, 10, 110, buffer);
    
    canvas_draw_str(canvas, 10, 130, "OK to Start");
    canvas_draw_str(canvas, 10, 150, "Up/Down: Adjust Channel");
    canvas_draw_str(canvas, 10, 170, "Left/Right: Change Power");
    canvas_draw_str(canvas, 10, 190, "Hold OK: Change Time");
    canvas_draw_str(canvas, 10, 210, "Hold Left: Scan Channels");
}

uint8_t scan_for_active_channel(uint8_t start, uint8_t end) {
    for(uint8_t ch = start; ch <= end; ch++) {
        if(nrf24_detect_signal(&nrf, ch)) {
            return ch;
        }
    }
    return start;
}

void input_callback(InputEvent* event, void* context) {
    if(event->type == InputTypeShort) {
        switch(event->key) {
            case InputKeyUp:
                if(end_channel < 100) end_channel++;
                break;
            case InputKeyDown:
                if(start_channel > 1) start_channel--;
                break;
            case InputKeyLeft:
                if(current_power_index > 0) current_power_index--;
                break;
            case InputKeyRight:
                if(current_power_index < POWER_LEVELS - 1) current_power_index++;
                break;
            case InputKeyOk:
                furi_thread_flags_set(furi_thread_get_id(), 1);
                break;
        }
    } else if(event->type == InputTypeLong) {
        if(event->key == InputKeyOk) {
            attack_duration = (attack_duration >= 60000) ? 5000 : attack_duration + 5000;
        } else if(event->key == InputKeyLeft) {
            target_channel = scan_for_active_channel(start_channel, end_channel);
        }
    }
}

int main(void) {
    Nrf24 nrf;
    if (!nrf24_init(&nrf, GPIO_PIN_X, GPIO_PIN_Y)) {
        printf("Erro ao inicializar o NRF24\n");
        return -1;
    }
    nrf24_set_power(&nrf, power_levels[current_power_index]);
    nrf24_set_mode(&nrf, NRF24_MODE_TX);

    Gui* gui = furi_record_open(RECORD_GUI);
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, update_screen, NULL);
    view_port_input_callback_set(view_port, input_callback, NULL);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    while(true) {
        if(furi_thread_flags_wait(1, FuriFlagWaitAny, FuriWaitForever) == 1) {
            furi_thread_flags_clear(1);
            nrf24_set_channel(&nrf, target_channel);
            send_jamming_signal(&nrf, target_channel, start_channel, end_channel, attack_duration, power_levels[current_power_index]);
        }
        furi_delay_ms(100);
    }

    return 0;
}
