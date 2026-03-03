#include "gesture.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "nrf_delay.h"
#include "nrfx_twim.h"
#include "microbit_v2.h"

// TWIM instance 0 on QWIIC pins, NULL handler = blocking transfers
static const nrfx_twim_t twim = NRFX_TWIM_INSTANCE(0);

// EasyDMA needs RAM buffers — keep these at file scope
static uint8_t tx_buf[2];
static uint8_t rx_buf;
static uint8_t fifo_buf[FIFO_MAX * 4];

// ---- I2C helpers ----

static void write_reg(uint8_t reg, uint8_t value) {
    tx_buf[0] = reg;
    tx_buf[1] = value;
    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TX(APDS9960_ADDR, tx_buf, 2);
    nrfx_twim_xfer(&twim, &xfer, 0);
}

static uint8_t read_reg(uint8_t reg) {
    tx_buf[0] = reg;
    // send register address, hold bus (no STOP between write and read)
    nrfx_twim_xfer_desc_t tx = NRFX_TWIM_XFER_DESC_TX(APDS9960_ADDR, tx_buf, 1);
    nrfx_twim_xfer(&twim, &tx, NRFX_TWIM_FLAG_TX_NO_STOP);
    // read back the value
    nrfx_twim_xfer_desc_t rx = NRFX_TWIM_XFER_DESC_RX(APDS9960_ADDR, &rx_buf, 1);
    nrfx_twim_xfer(&twim, &rx, 0);
    return rx_buf;
}

static void read_fifo(uint8_t count) {
    tx_buf[0] = REG_GFIFO_U;
    nrfx_twim_xfer_desc_t tx = NRFX_TWIM_XFER_DESC_TX(APDS9960_ADDR, tx_buf, 1);
    nrfx_twim_xfer(&twim, &tx, NRFX_TWIM_FLAG_TX_NO_STOP);
    nrfx_twim_xfer_desc_t rx = NRFX_TWIM_XFER_DESC_RX(APDS9960_ADDR, fifo_buf, count * 4);
    nrfx_twim_xfer(&twim, &rx, 0);
}

// ---- APDS9960 setup ----

static void sensor_init(void) {
    printf("APDS9960 ID: 0x%02X\n", read_reg(REG_ID));

    write_reg(REG_ENABLE, ENABLE_PON);  // power on
    nrf_delay_ms(10);

    write_reg(REG_GPENTH, 40); // enter gesture mode when proximity > 40
    write_reg(REG_GEXTH,  20); // exit gesture mode when proximity < 30
    // raw adc values (0-255), how much light is reflected back
    // 0 = no reflection, 255 = max reflection
    // might need to adjust

    // write_reg(REG_GCONF2, 0x01); // gesture gain 4x, LED 100mA, wait 2.8ms between samples            
    // write_reg(REG_PPULSE, 0x47); // proximity: 8us pulse length, 8 pulses (more IR power)             
    write_reg(REG_GPULSE, 0x88); // gesture:   16us pulse length, 9 pulses (more IR power)    
    write_reg(REG_GCONF4, GCONF4_GMODE); // enable gesture mode (no INT pin)
    write_reg(REG_ENABLE, ENABLE_PON | ENABLE_PEN | ENABLE_GEN);
}

// ---- Gesture direction decoder ----

// Threshold is a percentage (0-100) of asymmetry between opposing photodiodes.
// Using ratios instead of raw differences makes detection distance-independent.
#define GESTURE_THRESHOLD 30

static gesture_t decode_gesture(uint8_t count) {
    // find first and last valid FIFO entry
    // valid = no channel is zero, no channel is saturated (255), all channels >= 30
    int first = -1;
    int last  = -1;

    for (int i = 0; i < count; i++) {
        uint8_t u = fifo_buf[i*4 + 0];
        uint8_t d = fifo_buf[i*4 + 1];
        uint8_t l = fifo_buf[i*4 + 2];
        uint8_t r = fifo_buf[i*4 + 3];

        if (u == 255 || d == 255 || l == 255 || r == 255) { continue; } // saturated
        if (u < 30   || d < 30   || l < 30   || r < 30  ) { continue; } // too dim / noise

        if (first == -1) { first = i; }
        last = i;
    }

    if (first == -1 || first == last) {
        return GESTURE_NONE;
    }

    uint8_t fu = fifo_buf[first*4 + 0], fd = fifo_buf[first*4 + 1];
    uint8_t fl = fifo_buf[first*4 + 2], fr = fifo_buf[first*4 + 3];

    uint8_t lu = fifo_buf[last*4 + 0],  ld = fifo_buf[last*4 + 1];
    uint8_t ll = fifo_buf[last*4 + 2],  lr = fifo_buf[last*4 + 3];

    // ratio = (U-D)/(U+D) * 100  — percentage asymmetry, independent of distance
    int32_t f_r_ud = ((int32_t)(fu - fd) * 100) / (fu + fd);
    int32_t f_r_lr = ((int32_t)(fl - fr) * 100) / (fl + fr);
    int32_t l_r_ud = ((int32_t)(lu - ld) * 100) / (lu + ld);
    int32_t l_r_lr = ((int32_t)(ll - lr) * 100) / (ll + lr);

    int32_t delta_ud = l_r_ud - f_r_ud;
    int32_t delta_lr = l_r_lr - f_r_lr;

    // determine direction from whichever axis changed more
    int state_ud = (delta_ud >=  GESTURE_THRESHOLD) ?  1 :
                   (delta_ud <= -GESTURE_THRESHOLD) ? -1 : 0;
    int state_lr = (delta_lr >=  GESTURE_THRESHOLD) ?  1 :
                   (delta_lr <= -GESTURE_THRESHOLD) ? -1 : 0;

    if      (state_ud == -1 && state_lr ==  0) { return GESTURE_UP;    }
    else if (state_ud ==  1 && state_lr ==  0) { return GESTURE_DOWN;  }
    else if (state_ud ==  0 && state_lr == -1) { return GESTURE_LEFT;  }
    else if (state_ud ==  0 && state_lr ==  1) { return GESTURE_RIGHT; }

    // diagonal — pick the dominant axis
    if (abs(delta_ud) > abs(delta_lr)) {
        return (state_ud == -1) ? GESTURE_UP : GESTURE_DOWN;
    } else {
        return (state_lr == -1) ? GESTURE_LEFT : GESTURE_RIGHT;
    }
}

// ---- Public API ----

void gesture_init(void) {
    // set up I2C on QWIIC bus
    nrfx_twim_config_t twim_cfg = NRFX_TWIM_DEFAULT_CONFIG;
    twim_cfg.scl = I2C_QWIIC_SCL; // same as EDGE_P19
    twim_cfg.sda = I2C_QWIIC_SDA; // same as EDGE_P20
    nrfx_twim_init(&twim, &twim_cfg, NULL, NULL);
    nrfx_twim_enable(&twim);

    sensor_init();

    printf("gesture_init done\n");
}

gesture_t gesture_get(void) {
    // poll GSTATUS directly — no INT pin needed


    if (!(read_reg(REG_GSTATUS) & GSTATUS_GVALID)) {
        return GESTURE_NONE;
    }

    uint8_t count = read_reg(REG_GFLVL);
    if (count == 0) {
        return GESTURE_NONE;
    }
    if (count > FIFO_MAX) {
        count = FIFO_MAX;
    }

    // wait for the full swipe to complete before reading
    // so the FIFO has the whole gesture, not just the first half
    nrf_delay_ms(200);

    count = read_reg(REG_GFLVL);
    if (count == 0)       { return GESTURE_NONE; }
    if (count > FIFO_MAX) { count = FIFO_MAX; }

    read_fifo(count);
    write_reg(REG_GCONF4, GCONF4_GMODE | TEMP_GCONF4_GFIFO_CLR);

    gesture_t result = decode_gesture(count);

    // wait for gesture engine to go idle so stale data doesn't bleed into the next read
    // timeout after 1s in case hand stays in range
    uint32_t timeout = 1000;
    while ((read_reg(REG_GSTATUS) & GSTATUS_GVALID) && timeout > 0) {
        nrf_delay_ms(10);
        timeout -= 10;
    }

    return result;
}
