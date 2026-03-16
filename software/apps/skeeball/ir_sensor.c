#include "ir_sensor.h"

#include "nrfx_gpiote.h"
#include "microbit_v2.h"

static const uint32_t sensor_pins[NUM_SENSORS] = {
    EDGE_P1,   // IR_HOLE_10
    EDGE_P9,   // IR_HOLE_30
    EDGE_P12,  // IR_HOLE_50
};

static volatile bool triggered[NUM_SENSORS] = {false};

static void gpiote_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
    for (uint8_t i = 0; i < NUM_SENSORS; i++) {
        if (pin == sensor_pins[i]) {
            triggered[i] = true;
            break;
        }
    }
}

void ir_sensor_init(void) {
    if (!nrfx_gpiote_is_init()) {
        nrfx_gpiote_init();
    }

    nrfx_gpiote_in_config_t cfg = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    cfg.pull = NRF_GPIO_PIN_PULLUP;

    for (uint8_t i = 0; i < NUM_SENSORS; i++) {
        nrfx_gpiote_in_init(sensor_pins[i], &cfg, gpiote_handler);
        // start disabled — enabled only when game is PLAYING
    }
}

void ir_sensor_enable(void) {
    for (uint8_t i = 0; i < NUM_SENSORS; i++) {
        triggered[i] = false;  // clear any stale flags before enabling
        nrfx_gpiote_in_event_enable(sensor_pins[i], true);
    }
}

void ir_sensor_disable(void) {
    for (uint8_t i = 0; i < NUM_SENSORS; i++) {
        nrfx_gpiote_in_event_disable(sensor_pins[i]);
    }
}

bool ir_sensor_triggered(uint8_t sensor) {
    if (sensor >= NUM_SENSORS) return false;
    if (triggered[sensor]) {
        triggered[sensor] = false;
        return true;
    }
    return false;
}
