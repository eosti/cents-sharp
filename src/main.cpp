#include "tuner.h"

// Private functions
void __do_tuner();
void __do_metronome();
bool __metronome_irq(struct repeating_timer *t);
int64_t __metronome_off(alarm_id_t id, void *user_data);

uint16_t buffer1[CAPTURE_DEPTH];
uint16_t buffer2[CAPTURE_DEPTH];
fix15 fft_buffer[CAPTURE_DEPTH];

display_tuner_t tuner           = {0};
control_output_t control_output = {0};
tuner_mode_t tuner_mode         = MODE_TUNER;

struct repeating_timer metronome_timer;
volatile uint8_t metronome_counter;

void setup() {
    error_init();
    delay(4000);
    print_msg("Beginning Setup", INFO);

    mic_init(buffer1, buffer2);
    fft_init(fft_buffer, CAPTURE_BITS, MIC_SAMPLE_RATE);
    display_init();
    control_init(&control_output);
    print_msg("Setup complete!", INFO);
    tuner.center_frequency = 440;
    tuner.beat             = BEAT_NONE;
    tuner.metronome_bpm    = 60;
    pinMode(PIZEO_PIN, OUTPUT);

    // for(fix15 i = int2fix15(1); i < int2fix15(10000); i += int2fix15(1)) {
    //   char msg[64];
    //   uint8_t index;
    //   freq2note(i, &index, &(tuner.cents_deviation));
    //   tuner.current_note = (display_note_t)(index % 12);
    //   sprintf(msg, "Main current note: %d", index);
    //   print_msg(msg, DEBUG);
    //   display_tuner(&tuner);
    //   delay(10);
    // }

    // tuner.current_note = NOTE_A_FLAT;
    // for(int i = -50; i <= 50; i++) {
    //   tuner.cents_deviation = i;
    //   display_tuner(&tuner);
    //   delay(300);
    // }
}

void loop() {
    /* CHECK IF THE MODE HAS CHANGED */
    if (control_output.mode_but_pressed) {
        print_msg("Switching mode", DEBUG);
        tuner.mode_sel = 0;
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
        tuner.display_meme = false;
        __do_tuner();
    } else if (tuner_mode == MODE_TUNER_MEME) {
        tuner.display_meme = true;
        __do_tuner();
    } else if (tuner_mode == MODE_METRONOME) {
        __do_metronome();
    }
}

void __do_tuner() {
    // First, check if control state changes affect us
    if (control_output.encoder_movement) {
        if (!tuner.display_meme) {
            // Change center_frequency
            tuner.center_frequency += control_output.encoder_movement;
        } else {
            if (tuner.currency == CURRENCY_USD && control_output.encoder_movement < 0) {
                tuner.currency = CURRENCY_BTC;
            } else if (tuner.currency == CURRENCY_BTC && control_output.encoder_movement > 0) {
                tuner.currency = CURRENCY_USD;
            } else {
                tuner.currency = (currency_t)(tuner.currency + control_output.encoder_movement);
            }
        }
        control_output.encoder_movement = 0;
    }

    if (control_output.encoder_but_pressed) {
        // Doesn't do anything here
        control_output.encoder_but_pressed = 0;
    }

    fix15 result = do_fft();

    if (result != 0) {
        if (result == int2fix15(-1)) {
            // Low noise signal.
            tuner.low_noise = true;
        } 

        uint8_t index;
        freq2note(result, &index, &(tuner.cents_deviation));
        tuner.current_note = (display_note_t)(index % 12);
        tuner.low_noise = false;
        display_tuner(&tuner);
    }
}

void __do_metronome() {
    // First check if control state changed
    if (control_output.encoder_movement) {
        if (tuner.mode_sel == 0) {
            // Change BPM
            tuner.metronome_bpm += control_output.encoder_movement;
            if (tuner.metronome_bpm > 300) tuner.metronome_bpm = 300;
            if (tuner.metronome_bpm < 20) tuner.metronome_bpm = 20;
        } else if (tuner.mode_sel == 1) {
            if (tuner.beat == BEAT_NONE && control_output.encoder_movement < 0) {
                tuner.beat = BEAT_SPLIT_SIXTEEN;
            } else {
                tuner.beat = (display_beat_t)(tuner.beat + control_output.encoder_movement);
                if (tuner.beat > BEAT_SPLIT_SIXTEEN) tuner.beat = BEAT_NONE;
            }
        }
        char msg[64];
        sprintf(msg, "metronome: mode %d at %dbpm", tuner.beat, tuner.metronome_bpm);
        print_msg(msg, INFO);

        control_output.encoder_movement = 0;

        // Something changed, so we have to reset the timers
        cancel_repeating_timer(&metronome_timer);
        if (tuner.beat != BEAT_NONE) {
            uint32_t interval = 60 * 1000 / tuner.metronome_bpm;
            metronome_counter = 0;
            if (tuner.beat <= BEAT_9) {
                add_repeating_timer_ms(-interval, __metronome_irq, NULL, &metronome_timer);
            } else if (tuner.beat == BEAT_EIGHTH) {
                add_repeating_timer_ms(-(interval / 2), __metronome_irq, NULL, &metronome_timer);
            } else if (tuner.beat == BEAT_TRIPLET || tuner.beat == BEAT_SPLIT_TRIPLET) {
                add_repeating_timer_ms(-(interval / 3), __metronome_irq, NULL, &metronome_timer);
            } else if (tuner.beat == BEAT_SIXTEEN || tuner.beat == BEAT_SPLIT_SIXTEEN) {
                add_repeating_timer_ms(-(interval / 4), __metronome_irq, NULL, &metronome_timer);
            } else {
                fatal_error("invalid tuner beat");
            }
        }
    }

    if (control_output.encoder_but_pressed) {
        tuner.mode_sel                     = tuner.mode_sel ? 0 : 1;
        control_output.encoder_but_pressed = 0;
    }

    display_metronome(&tuner);
}

bool __metronome_irq(struct repeating_timer *t) {
    print_msg("metronome_irq", DEBUG);
    // What kind of tone?
    if (tuner.beat == BEAT_0) {
        tone(PIZEO_PIN, METRONOME_LOW_TONE);
    } else if (tuner.beat == BEAT_1) {
        tone(PIZEO_PIN, METRONOME_HIGH_TONE);
    } else if (tuner.beat >= BEAT_2 && tuner.beat <= BEAT_9) {
        if (metronome_counter == 0) {
            tone(PIZEO_PIN, METRONOME_HIGH_TONE);
        } else {
            tone(PIZEO_PIN, METRONOME_LOW_TONE);
        }

        if (metronome_counter == tuner.beat - 2) {
            metronome_counter = 0;
        } else {
            metronome_counter++;
        }
    } else if (tuner.beat == BEAT_EIGHTH) {
        if (metronome_counter == 0) {
            tone(PIZEO_PIN, METRONOME_HIGH_TONE);
        } else {
            tone(PIZEO_PIN, METRONOME_LOW_TONE);
        }

        metronome_counter = metronome_counter ? 0 : 1;
    } else if (tuner.beat >= BEAT_TRIPLET) {
        if (metronome_counter == 0) {
            tone(PIZEO_PIN, METRONOME_HIGH_TONE);
        } else if (metronome_counter == 1 && (tuner.beat == BEAT_SPLIT_TRIPLET || tuner.beat == BEAT_SPLIT_SIXTEEN)) {
            noTone(PIZEO_PIN);
        } else if (metronome_counter == 2 && tuner.beat == BEAT_SPLIT_SIXTEEN) {
            noTone(PIZEO_PIN);
        } else {
            tone(PIZEO_PIN, METRONOME_LOW_TONE);
        }

        if (tuner.beat == BEAT_SPLIT_TRIPLET || tuner.beat == BEAT_TRIPLET) {
            if (metronome_counter == 2) {
                metronome_counter = 0;
            } else {
                metronome_counter++;
            }
        } else if (tuner.beat == BEAT_SPLIT_SIXTEEN || tuner.beat == BEAT_SIXTEEN) {
            if (metronome_counter == 3) {
                metronome_counter = 0;
            } else {
                metronome_counter++;
            }
        }
    }

    // Set alarm to trigger the off
    add_alarm_in_ms(METRONOME_TONE_TIME, __metronome_off, NULL, false);
    return true;
}

int64_t __metronome_off(alarm_id_t id, void *user_data) {
    noTone(PIZEO_PIN);
    return 0;
}