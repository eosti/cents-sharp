#pragma once
#include "error.h"
#include "fix.h"
#include "hardware/dma.h"
#include "pico/stdlib.h"

#include <Arduino.h>

/* CONSTANTS */
#define NUM_OCTAVES            8
#define ROLLING_ITEMS          16
#define ROLLING_OUTLIER_THRESH 4 // number of entries before buffers swapped
#define ROLLING_DEVIANCE_MULT  0.3
#define LOW_NOISE_THRESH       30

/* EXPORTED FUNTIONS */
void fft_init(fix15 *fft_output, uint16_t num_bits, uint32_t samplerate);
fix15 do_fft();
void freq2note(fix15 freq, uint8_t *note_index, int8_t *cents_deviation);
void change_fft_center(uint16_t new_center);