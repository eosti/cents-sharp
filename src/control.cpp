#include "control.h"

// Private prototypes
bool __control_poll(struct repeating_timer *t);
void __read_control();

control_state_t last_state = {0}, cur_state = {0};
control_output_t *output_target;

struct repeating_timer control_timer;

void control_init(control_output_t *out) {
    // Button is connected to pin and gnd
    pinMode(MODE_BUT, INPUT_PULLUP);

    // Encoder
    pinMode(ENCODER_CLK, INPUT);
    pinMode(ENCODER_DAT, INPUT);
    pinMode(ENCODER_BUT, INPUT);

    output_target = out;
    __read_control();

    add_repeating_timer_ms(-CONTROL_POLL_INTERVAL, __control_poll, NULL, &control_timer);
}

bool __control_poll(struct repeating_timer *t) {
    __read_control();

    // Check on buttons (change state on release)
    if (last_state.mode_but_state == 1 && cur_state.mode_but_state == 0) {
        output_target->mode_but_pressed++;
        print_msg("control: mode_but pressed", DEBUG);
    } 

    if (last_state.encoder_but_state == 1 && cur_state.encoder_but_state == 0) {
        output_target->encoder_but_pressed++;
        print_msg("control: encoder_but pressed", DEBUG);
    }

    // Check on encoders
    if (last_state.encoder_a_state == 1 && cur_state.encoder_a_state == 0) {
        if (cur_state.encoder_a_state == cur_state.encoder_b_state) {
            output_target->encoder_movement--;
        } else {
            output_target->encoder_movement++;
        }
        print_msg("control: encoder state change detected", DEBUG);
    }
    return true;
}

void __read_control() {
    // Copy old state into last state
    last_state.encoder_a_state      = cur_state.encoder_a_state;
    last_state.encoder_b_state      = cur_state.encoder_b_state;
    last_state.encoder_but_state    = cur_state.encoder_but_state;
    last_state.mode_but_state       = cur_state.mode_but_state;

    // Get new state
    cur_state.encoder_a_state = digitalRead(ENCODER_CLK);
    cur_state.encoder_b_state = digitalRead(ENCODER_DAT);
    cur_state.encoder_but_state = digitalRead(ENCODER_BUT);
    cur_state.mode_but_state = !digitalRead(MODE_BUT);
}