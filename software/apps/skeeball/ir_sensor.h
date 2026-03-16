#pragma once

#include <stdint.h>
#include <stdbool.h>

// Sensor indices
#define IR_HOLE_10   0   // EDGE_P1  — 10 pts
#define IR_HOLE_30   1   // EDGE_P9  — 30 pts
#define IR_HOLE_50   2   // EDGE_P12 — 50 pts
#define NUM_SENSORS  3

void ir_sensor_init(void);

// Enable/disable all IR interrupts. enable during PLAYING state
void ir_sensor_enable(void);
void ir_sensor_disable(void);

// Returns true and clears flag if sensor fired since last call, so it doesnt double fire
bool ir_sensor_triggered(uint8_t sensor);
