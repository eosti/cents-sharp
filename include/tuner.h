#pragma once
#include "control.h"
#include "display.h"
#include "error.h"
#include "fft.h"
#include "fix.h"
#include "mic.h"

#include <Arduino.h>

/* Types */

enum tuner_mode_t { MODE_TUNER, MODE_TUNER_MEME, MODE_METRONOME };

/* CONSTANTS */
#define PIZEO_PIN           15
#define METRONOME_LOW_TONE  880
#define METRONOME_HIGH_TONE 1760
#define METRONOME_TONE_TIME 100 // in ms