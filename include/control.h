#pragma once
#include "error.h"
#include "pico/stdlib.h"

#include <Arduino.h>

/* TYPEDEF */
typedef struct control_state_t {
    bool mode_but_state;
    bool encoder_a_state;
    bool encoder_b_state;
    bool encoder_but_state;
} control_state_t;

typedef struct control_output_t {
    uint8_t mode_but_pressed;
    uint8_t encoder_but_pressed;
    int8_t encoder_movement;
} control_output_t;

/* DEFINES */
// Pin definitions
#define ENCODER_CLK 4
#define ENCODER_DAT 3
#define ENCODER_BUT 2
#define MODE_BUT    5

// In ms, affects debouncing
#define CONTROL_POLL_INTERVAL 3

/* EXPORTED FUNCTIONS */
void control_init(control_output_t *out);