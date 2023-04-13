#include "tuner.h"

// Global variables
uint16_t buffer1[CAPTURE_DEPTH];
uint16_t buffer2[CAPTURE_DEPTH];
fix15 fft_buffer[CAPTURE_DEPTH];

control_output_t control_output = {0};
tuner_mode_t tuner_mode         = MODE_TUNER;

void setup() {
    error_init();
    if (MSG_LEVEL == DEBUG) delay(4000); // Wait for serial monitor to start
    print_msg("Beginning Setup", INFO);

    mic_init(buffer1, buffer2);
    fft_init(fft_buffer, CAPTURE_BITS, MIC_SAMPLE_RATE);
    display_init();
    control_init(&control_output);
    tuner_init();

    print_msg("Setup complete!", INFO);
}

void loop() {
    /* CHECK IF THE MODE HAS CHANGED */
    if (control_output.mode_but_pressed) {
        tuner_new_mode();
        control_output.mode_but_pressed--;
        switch (tuner_mode) {
            case MODE_TUNER:
                tuner_mode = MODE_TUNER_MEME;
                print_msg("mode switch: tuner meme", INFO);
                break;
            case MODE_TUNER_MEME:
                tuner_mode = MODE_METRONOME;
                print_msg("mode switch: metronome", INFO);
                break;
            case MODE_METRONOME:
                tuner_mode = MODE_TUNER;
                print_msg("mode switch: tuner", INFO);
                break;
            default:
                tuner_mode = MODE_TUNER;
        }
    }

    /* TAKE ACTION BASED ON MODE */
    if (tuner_mode == MODE_TUNER) {
        tuner_meme(false);
        do_tuner(&control_output);
    } else if (tuner_mode == MODE_TUNER_MEME) {
        tuner_meme(true);
        do_tuner(&control_output);
    } else if (tuner_mode == MODE_METRONOME) {
        do_metronome(&control_output);
    }
}