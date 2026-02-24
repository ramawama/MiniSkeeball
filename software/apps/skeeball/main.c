// Skeeball game for micro:bit v2
// CE346 - Northwestern University

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "microbit_v2.h"

int main(void) {
    // Configure edge connector pin 1 as input with internal pull-up resistor.
    // Pull-up ensures the pin reads HIGH when the beam is intact and the
    // phototransistor is off (open-collector output floats to VCC via pull-up).
    nrf_gpio_cfg_input(EDGE_P1, NRF_GPIO_PIN_PULLUP);
    printf("IR break beam test started\n");

    // Read the initial pin state so the first loop iteration has a valid
    // baseline and doesn't fire a spurious transition on startup.
    uint32_t prev_state = nrf_gpio_pin_read(EDGE_P1);

    while (true) {
        uint32_t state = nrf_gpio_pin_read(EDGE_P1);

        // Pin LOW (0): phototransistor conducting → beam is broken (ball present).
        // Pin HIGH (1): phototransistor off → beam is intact (no obstruction).
        if (state == 0 && prev_state == 1) {
            printf("Beam broken!\n");
        } else if (state == 1 && prev_state == 0) {
            printf("Beam restored.\n");
        }

        // Save current state to detect the next transition on the following poll.
        prev_state = state;

        // Poll every 100 ms.
        // In game use interupts instead of constant polling for better performance and lower power consumption.

        nrf_delay_ms(100);
    }
}
