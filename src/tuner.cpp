#include "tuner.h"

// Private functions
bool __metronome_irq(struct repeating_timer *t);
int64_t __metronome_off(alarm_id_t id, void *user_data);
static inline display_note_t __noteindex2displaynote(uint8_t index);
static uint32_t __note2freq(display_note_t note, uint8_t octave);

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
    tuner->soundback_en = false;
    tuner->soundback_note = NOTE_A;
    tuner->soundback_octave = 4;
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
        tuner->current_note = __noteindex2displaynote(index);
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
            if (tuner->soundback_en) {
                // Cannot use soundback and metronome at the same time
            } else {
                if (tuner->beat == BEAT_NONE && control_output->encoder_movement < 0) {
                    tuner->beat = BEAT_SPLIT_SIXTEEN;
                } else {
                    tuner->beat = (display_beat_t)(tuner->beat + control_output->encoder_movement);
                    if (tuner->beat > BEAT_SPLIT_SIXTEEN) tuner->beat = BEAT_NONE;
                }
            }
        }
        char msg[64];
        sprintf(msg, "metronome: mode %d at %dbpm", tuner->beat, tuner->metronome_bpm);
        print_msg(msg, DEBUG);

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

void do_soundback(control_output_t *control_output) {
    // Check if control state changed
    if (control_output->encoder_movement) {
        if (tuner->soundback_note == NOTE_A_FLAT && control_output->encoder_movement < 0) {
            if (tuner->soundback_octave == 1) {
                // Can't go lower than octave 1
            } else {
                tuner->soundback_note = NOTE_G;
                tuner->soundback_octave--;
            }
            control_output->encoder_movement++;
        } else if (tuner->soundback_note == NOTE_G && control_output->encoder_movement > 0) {
            if (tuner->soundback_octave == 7) {
                // Can't go higher than octave 7
            } else {
                tuner->soundback_note = NOTE_A_FLAT;
                tuner->soundback_octave++;
            }
            control_output->encoder_movement--;
        } else {
            tuner->soundback_note = (display_note_t)(tuner->soundback_note + control_output->encoder_movement);
            // ack, constrain if we go too fast
            if (tuner->soundback_note >= NOTE_NONE) tuner->soundback_note = NOTE_G;
            control_output->encoder_movement = 0;
        }
        // Play new tone
        if (tuner->soundback_en) {
            tone(PIZEO_PIN, __note2freq(tuner->soundback_note, tuner->soundback_octave));
        }
    }

    if (control_output->encoder_but_pressed) {
        if (tuner->beat != BEAT_NONE) {
            // Cannot use soundback and metronome at same time
        } else {
            tuner->soundback_en = tuner->soundback_en ? 0 : 1;
        }

        // 
        if (tuner->soundback_en) {
            tone(PIZEO_PIN, __note2freq(tuner->soundback_note, tuner->soundback_octave));
        } else if (tuner->beat == BEAT_NONE) {
            noTone(PIZEO_PIN);
        }

        control_output->encoder_but_pressed = 0;
    }

    display_soundback(tuner);
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

static inline display_note_t __noteindex2displaynote(uint8_t index) {
    // The index is aligned to C0, whereas the display index is aligned to A flat
    return (display_note_t)((index + 4) % 12);
}

static uint32_t __note2freq(display_note_t note, uint8_t octave) {
    return index2freq(note - 4 + octave * 12);
}