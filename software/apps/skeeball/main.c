// Skeeball game for micro:bit v2
// CE346 - Northwestern University

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf_delay.h"
#include "microbit_v2.h"
#include "gesture.h"
#include "display.h"
#include "ir_sensor.h"
#include "game.h"

int main(void) {
    printf("Skeeball started\n");

    display_init();
    ir_sensor_init();
    gesture_init();
    game_init();

    while (true) {
        // Check all IR sensors
        for (uint8_t i = 0; i < NUM_SENSORS; i++) {
            if (ir_sensor_triggered(i)) {
                game_sensor_event(i);
            }
        }

        // Gesture cycles menu, proximity hover selects
        gesture_t g = gesture_get();
        uint8_t prox = gesture_proximity();
        game_input(g != GESTURE_NONE, prox);

        // Handle timer ticks and game over
        game_tick();

        nrf_delay_ms(10);
    }
}
