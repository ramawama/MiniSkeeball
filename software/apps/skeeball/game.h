#pragma once

#include <stdint.h>

void game_init(void);

// Call every main loop iteration — handles menu, scoring, timer display
void game_tick(void);

// Call when an IR sensor fires
void game_sensor_event(uint8_t sensor);

// Call with latest gesture and proximity each loop
void game_input(uint8_t gesture_fired, uint8_t proximity);
