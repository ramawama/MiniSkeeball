#include "game.h"

#include <stdio.h>
#include "display.h"
#include "ir_sensor.h"

// ---------------------------------------------------------------------------
// Scoring
// ---------------------------------------------------------------------------
static const uint16_t sensor_points[NUM_SENSORS] = {
    10,   // IR_HOLE_10
    30,   // IR_HOLE_30
    50,   // IR_HOLE_50
};

#define PROX_SELECT  30    // proximity threshold to select menu item

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
typedef enum { STATE_MENU, STATE_PLAYING, STATE_GAME_OVER } game_state_t;
typedef enum { MENU_6BALL, MENU_9BALL }   menu_opt_t;

static game_state_t state      = STATE_MENU;
static menu_opt_t   menu_opt   = MENU_6BALL;
static uint32_t     score      = 0;
static uint8_t      balls_max  = 0;
static uint8_t      balls_used = 0;

#define DEBOUNCE_TICKS  10  // 10 * 10ms = 100ms lockout per sensor

static uint8_t sensor_debounce[NUM_SENSORS] = {0};

// Set by game_sensor_event, handled in game_tick()
static volatile bool game_over_pending = false;

// Proximity debounce — only select once per hover
static bool prox_was_high = false;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
static void show_menu(void) {
    if (menu_opt == MENU_6BALL) {
        display_show_text("6 BALL\nHover=Select");
    } else {
        display_show_text("9 BALL\nHover=Select");
    }
}

static void start_game(uint8_t num_balls) {
    score             = 0;
    balls_max         = num_balls;
    balls_used        = 0;
    game_over_pending = false;
    for (uint8_t i = 0; i < NUM_SENSORS; i++) sensor_debounce[i] = 0;
    state                = STATE_PLAYING;
    ir_sensor_enable();
    char buf[16];
    snprintf(buf, sizeof(buf), "GO!\n%u BALLS", num_balls);
    display_show_text(buf);
    printf("Game started: %u balls\n", num_balls);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void game_init(void) {
    show_menu();
    printf("game: ready\n");
}

void game_sensor_event(uint8_t sensor) {
    if (state != STATE_PLAYING) return;
    if (sensor >= NUM_SENSORS) return;
    if (sensor_debounce[sensor] > 0) return;

    sensor_debounce[sensor] = DEBOUNCE_TICKS;
    balls_used++;

    uint16_t pts = sensor_points[sensor];
    score += pts;
    printf("+%u pts (sensor %u) — total %lu [ball %u/%u]\n",
           pts, sensor, score, balls_used, balls_max);

    display_flash();

    if (balls_used >= balls_max) {
        game_over_pending = true;
        return;
    }

    char buf[24];
    uint8_t left = balls_max - balls_used;
    snprintf(buf, sizeof(buf), "%lu\n%u left", (unsigned long)score, left);
    display_show_text(buf);
    printf("Score: %lu  Balls left: %u\n", (unsigned long)score, balls_max - balls_used);
}

void game_input(uint8_t gesture_fired, uint8_t proximity) {
    bool prox_high          = (proximity > PROX_SELECT);
    bool prox_just_triggered = prox_high && !prox_was_high;
    prox_was_high = prox_high;

    if (state == STATE_MENU) {
        if (prox_just_triggered) {
            start_game((menu_opt == MENU_6BALL) ? 6 : 9);
        } else if (gesture_fired) {
            menu_opt = (menu_opt == MENU_6BALL) ? MENU_9BALL : MENU_6BALL;
            show_menu();
        }
    } else if (state == STATE_GAME_OVER) {
        if (gesture_fired || prox_just_triggered) {
            state = STATE_MENU;
            show_menu();
        }
    }
}

void game_tick(void) {
    for (uint8_t i = 0; i < NUM_SENSORS; i++) {
        if (sensor_debounce[i] > 0) sensor_debounce[i]--;
    }

    if (game_over_pending) {
        game_over_pending = false;
        state = STATE_GAME_OVER;
        ir_sensor_disable();
        printf("Game over! Score: %lu\n", (unsigned long)score);
        char buf[24];
        snprintf(buf, sizeof(buf), "SCORE\n%lu", (unsigned long)score);
        display_show_text(buf);
    }
}
