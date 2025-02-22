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
    canvas_draw_str(canvas, 10, 30, "Start Channel:");
    canvas_draw_number(canvas, 150, 30, start_channel);
    canvas_draw_str(canvas, 10, 50, "End Channel:");
    canvas_draw_number(canvas, 150, 50, end_channel);
    canvas_draw_str(canvas, 10, 70, "Power Level:");
    canvas_draw_number(canvas, 150, 70, current_power_index);
    canvas_draw_str(canvas, 10, 90, "Attack Time (s):");
    canvas_draw_number(canvas, 150, 90, attack_duration / 1000);
    canvas_draw_str(canvas, 10, 110, "Target Channel:");
    canvas_draw_number(canvas, 150, 110, target_channel);
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
        if(event->key == InputKeyUp && end_channel < 100) {
            end_channel++;
        } else if(event->key == InputKeyDown && start_channel > 1) {
            start_channel--;
        } else if(event->key == InputKeyLeft && current_power_index > 0) {
            current_power_index--;
        } else if(event->key == InputKeyRight && current_power_index < POWER_LEVELS - 1) {
            current_power_index++;
        } else if(event->key == InputKeyOk) {
            furi_thread_flags_set(furi_thread_get_id(), 1);
        }
    } else if(event->type == InputTypeLong && event->key == InputKeyOk) {
        attack_duration += 5000;
        if(attack_duration > 60000) attack_duration = 5000;
    } else if(event->type == InputTypeLong && event->key == InputKeyLeft) {
        target_channel = scan_for_active_channel(start_channel, end_channel);
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
            send_jamming_signal(&nrf, target_channel, start_channel, end_channel, attack_duration, power_levels[current_power_index]);
        }
        furi_delay_ms(100);
    }

    return 0;
}
