// PWM Servo App
//
// Use PWM to control a servo

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "nrfx_pwm.h"

#include "microbit_v2.h"

// PWM configuration
static const nrfx_pwm_t PWM_INST = NRFX_PWM_INSTANCE(0);

// Holds duty cycle values to trigger PWM toggle
nrf_pwm_values_common_t sequence_data[1] = {0};

// Sequence structure for configuring DMA
nrf_pwm_sequence_t pwm_sequence = {
  .values.p_common = sequence_data,
  .length = 1,
  .repeats = 0,
  .end_delay = 0,
};


static void pwm_init(void) {
  // Initialize the PWM
  // This time EDGE_P0 should be your output pin
  // Set the clock to 500 kHz, count mode to Up, and load mode to Common
  // This time the COUNTERTOP value DOES matter. It should be configured for a 20 ms period.
  // TODO
  //500 kHz / 20ms = 10000 counts
  nrfx_pwm_config_t config = {
      .output_pins = {
          EDGE_P0,
          NRFX_PWM_PIN_NOT_USED,
          NRFX_PWM_PIN_NOT_USED,
          NRFX_PWM_PIN_NOT_USED,
      },
      .irq_priority = NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY,
      .base_clock = NRF_PWM_CLK_500kHz,
      .count_mode = NRF_PWM_MODE_UP,
      .top_value = 10000, // 20 ms period at 500 kHz
      .load_mode = NRF_PWM_LOAD_COMMON,
      .step_mode = NRF_PWM_STEP_AUTO,
  };

  nrfx_pwm_init(&PWM_INST, &config, NULL);
}

// Takes in an angle from 0-180 and sets the servo to that angle
static void set_servo(uint8_t angle) {
  // Limit to 0 to 180 degrees
  if (angle > 180) {
    angle = 180;
  }

  // Calculate the duration in milliseconds based on the angle
  // For our specific servo:
  //   2.250 ms = 180 degrees
  //   1.375 ms =  90 degrees
  //   0.500 ms =   0 degrees
  // Other angles are linearly mapped between these
  // TODO

  // 500 ms + (angle * (slope) / 180)
  uint32_t slope = 2250 - 500; // 2.250 ms - 0.500 ms = 1.750 ms
  uint32_t duration = 500 + (angle * slope / 180);


  // Calculate duty cycle based on duration (knowing the period of 20 ms)
  // TODO
  // Duty cycle = (duration / period) * countertop
  uint32_t duty_cycle = (duration * 10000) / 20000;

  // Modify the sequence data to match the duty cycle
  // HOWEVER, to control a servo, the PWM output MUST be left-aligned
  // To do so, the Most Significant bit of the 16-bit value must be set to 1
  // TODO
  sequence_data[0] = duty_cycle | (1 << 15); // Set MSB to 1 for left alignment

  // Start playback of the samples and loop indefinitely
  // TODO
  nrfx_pwm_simple_playback(&PWM_INST, &pwm_sequence, 1, NRF_PWM_MODE_UP); 
}


int main(void) {
  printf("Board started!\n");

  // initialize PWM
  pwm_init();

  // Continuously vary the servo angle
  // Once per second it should update by 45 degrees
  // It should range from 0 to 180 degrees
  // TODO

  while (1) {
    for (uint8_t angle = 0; angle <= 180; angle += 45) {
      set_servo(angle);
      nrf_delay_ms(1000);
    }
  }

}

