#pragma once

#include <stdint.h>
#include <stdbool.h>

// APDS-9960 I2C address from datasheet
#define APDS9960_ADDR  0x39

// ---- Register map ----
#define REG_ENABLE    0x80
#define REG_PPULSE    0x8E   // proximity pulse count/length
#define REG_CONTROL   0x8F   // gain control
#define REG_ID        0x92   // chip ID 
#define REG_GPENTH    0xA0   // gesture proximity entry threshold
#define REG_GEXTH     0xA1   // gesture exit threshold
#define REG_GCONF1    0xA2   // exit threshold (7:6)/ fifo threshold (5:2) / exit persistence (1:0) 
#define REG_GCONF2    0xA3   // gain (6:5)/ LED drive (4:3)/ wait time(2:0)
#define REG_GPULSE    0xA6   // gesture pulse count (5:0)/ pulse length (7:6)
#define REG_GCONF3    0xAA   // gesture dim select
#define REG_GCONF4    0xAB   // gesture mode (0)/ interrupt (1)
#define REG_GFLVL     0xAE   // FIFO lvl (# datasets in buffer)
#define REG_GSTATUS   0xAF   // gesture status flags (1: GVALID, 0: GFIFO_U/D/L/R)
#define REG_GFIFO_U   0xFC   // burst-read entry for FIFO (Up xFC/Down xFD/Left xFE/Right xFF) 

// ENABLE register bits
#define ENABLE_PON   (1 << 0) // power ON
#define ENABLE_PEN   (1 << 2) // proximity enable
#define ENABLE_GEN   (1 << 6) // gesture enable

// GSTATUS bits
#define GSTATUS_GVALID  (1 << 0)

// GCONF4 bits
#define GCONF4_GMODE     (1 << 0) // gesture mode
#define GCONF4_GIEN      (1 << 1) // gesture interrupt enable
#define TEMP_GCONF4_GFIFO_CLR (1 << 2) // clear

// Maximum datasets the hardware FIFO can hold
#define FIFO_MAX  32

// direction of gesture
typedef enum { 
    GESTURE_NONE,
    GESTURE_UP,
    GESTURE_DOWN,
    GESTURE_LEFT,
    GESTURE_RIGHT,
} gesture_t;

// initialize for gesture detection
void gesture_init(void);

// get current gesture.  Returns GESTURE_NONE if nothing new.
gesture_t gesture_get(void);
