#include "tuner.h"

// Private functions
bool __metronome_irq(struct repeating_timer *t);
int64_t __metronome_off(alarm_id_t id, void *user_data);

// Global variables
struct display_tuner_t *tuner;
struct repeating_timer metronome_timer;
volatile uint8_t metronome_counter;

/**
 * @brief Init tuner functions
 *
 */
void tuner_init() {
    tuner                   = (display_tuner_t *)(malloc(sizeof(struct display_tuner_t)));
    tuner->center_frequency = 440;
    tuner->beat             = BEAT_NONE;
    tuner->metronome_bpm    = 60;
    tuner->currency         = CURRENCY_USD;
    change_fft_center(tuner->center_frequency);
    pinMode(PIZEO_PIN, OUTPUT);
}

/**
 * @brief Do tuner actions (determine note, display)
 *
 * @param control_output physical control state
 */
void do_tuner(control_output_t *control_output) {
    // First, check if control state changes affect us
    if (control_output->encoder_movement) {
        if (!tuner->display_meme) {
            // Change center_frequency
            tuner->center_frequency += control_output->encoder_movement;
            change_fft_center(tuner->center_frequency);
        } else {
            if (tuner->currency == CURRENCY_USD && control_output->encoder_movement < 0) {
                tuner->currency = CURRENCY_BTC;
            } else if (tuner->currency == CURRENCY_BTC && control_output->encoder_movement > 0) {
                tuner->currency = CURRENCY_USD;
            } else {
                tuner->currency = (currency_t)(tuner->currency + control_output->encoder_movement);
            }
        }
        control_output->encoder_movement = 0;
    }

    if (control_output->encoder_but_pressed) {
        // Doesn't do anything here
        control_output->encoder_but_pressed = 0;
    }

    fix15 result = do_fft();

    if (result != 0) {
        if (result == int2fix15(-1)) {
            // Low noise signal.
            tuner->low_noise = true;
        }

        uint8_t index;
        freq2note(result, &index, &(tuner->cents_deviation));
        tuner->current_note = (display_note_t)(index % 12);
        tuner->low_noise    = false;
        display_tuner(tuner);
    }
}

/**
 * @brief Do metronome actions (config metronome, display)
 *
 * @param control_output physical controls state
 */
void do_metronome(control_output_t *control_output) {
    // First check if control state changed
    if (control_output->encoder_movement) {
        if (tuner->mode_sel == 0) {
            // Change BPM
            tuner->metronome_bpm += control_output->encoder_movement;
            if (tuner->metronome_bpm > 300) tuner->metronome_bpm = 300;
            if (tuner->metronome_bpm < 20) tuner->metronome_bpm = 20;
        } else if (tuner->mode_sel == 1) {
            if (tuner->beat == BEAT_NONE && control_output->encoder_movement < 0) {
                tuner->beat = BEAT_SPLIT_SIXTEEN;
            } else {
                tuner->beat = (display_beat_t)(tuner->beat + control_output->encoder_movement);
                if (tuner->beat > BEAT_SPLIT_SIXTEEN) tuner->beat = BEAT_NONE;
            }
        }
        char msg[64];
        sprintf(msg, "metronome: mode %d at %dbpm", tuner->beat, tuner->metronome_bpm);
        print_msg(msg, INFO);

        control_output->encoder_movement = 0;

        // Something changed, so we have to reset the timers
        cancel_repeating_timer(&metronome_timer);
        if (tuner->beat != BEAT_NONE) {
            uint32_t interval = 60 * 1000 / tuner->metronome_bpm;
            metronome_counter = 0;
            if (tuner->beat <= BEAT_9) {
                add_repeating_timer_ms(-interval, __metronome_irq, NULL, &metronome_timer);
            } else if (tuner->beat == BEAT_EIGHTH) {
                add_repeating_timer_ms(-(interval / 2), __metronome_irq, NULL, &metronome_timer);
            } else if (tuner->beat == BEAT_TRIPLET || tuner->beat == BEAT_SPLIT_TRIPLET) {
                add_repeating_timer_ms(-(interval / 3), __metronome_irq, NULL, &metronome_timer);
            } else if (tuner->beat == BEAT_SIXTEEN || tuner->beat == BEAT_SPLIT_SIXTEEN) {
                add_repeating_timer_ms(-(interval / 4), __metronome_irq, NULL, &metronome_timer);
            } else {
                fatal_error("invalid tuner beat");
            }
        }
    }

    if (control_output->encoder_but_pressed) {
        tuner->mode_sel                     = tuner->mode_sel ? 0 : 1;
        control_output->encoder_but_pressed = 0;
    }

    display_metronome(tuner);
}

/**
 * @brief IRQ handler for metronome timer
 *
 */
bool __metronome_irq(struct repeating_timer *t) {
    print_msg("metronome_irq", DEBUG);
    // What kind of tone?
    if (tuner->beat == BEAT_0) {
        tone(PIZEO_PIN, METRONOME_LOW_TONE);
    } else if (tuner->beat == BEAT_1) {
        tone(PIZEO_PIN, METRONOME_HIGH_TONE);
    } else if (tuner->beat >= BEAT_2 && tuner->beat <= BEAT_9) {
        if (metronome_counter == 0) {
            tone(PIZEO_PIN, METRONOME_HIGH_TONE);
        } else {
            tone(PIZEO_PIN, METRONOME_LOW_TONE);
        }

        if (metronome_counter == tuner->beat - 2) {
            metronome_counter = 0;
        } else {
            metronome_counter++;
        }
    } else if (tuner->beat == BEAT_EIGHTH) {
        if (metronome_counter == 0) {
            tone(PIZEO_PIN, METRONOME_HIGH_TONE);
        } else {
            tone(PIZEO_PIN, METRONOME_LOW_TONE);
        }

        metronome_counter = metronome_counter ? 0 : 1;
    } else if (tuner->beat >= BEAT_TRIPLET) {
        if (metronome_counter == 0) {
            tone(PIZEO_PIN, METRONOME_HIGH_TONE);
        } else if (metronome_counter == 1 && (tuner->beat == BEAT_SPLIT_TRIPLET || tuner->beat == BEAT_SPLIT_SIXTEEN)) {
            noTone(PIZEO_PIN);
        } else if (metronome_counter == 2 && tuner->beat == BEAT_SPLIT_SIXTEEN) {
            noTone(PIZEO_PIN);
        } else {
            tone(PIZEO_PIN, METRONOME_LOW_TONE);
        }

        if (tuner->beat == BEAT_SPLIT_TRIPLET || tuner->beat == BEAT_TRIPLET) {
            if (metronome_counter == 2) {
                metronome_counter = 0;
            } else {
                metronome_counter++;
            }
        } else if (tuner->beat == BEAT_SPLIT_SIXTEEN || tuner->beat == BEAT_SIXTEEN) {
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

/**
 * @brief alarm handler for turning metronome off
 *
 */
int64_t __metronome_off(alarm_id_t id, void *user_data) {
    noTone(PIZEO_PIN);
    return 0;
}

/**
 * @brief Resets tuner state during a mode switch
 *
 */
void tuner_new_mode() {
    tuner->mode_sel = 0;
}

/**
 * @brief determines if there's funny business or not
 *
 * @param meme
 */
void tuner_meme(bool meme) {
    tuner->display_meme = meme;
}