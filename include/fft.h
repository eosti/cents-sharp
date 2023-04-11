#pragma once
#include <Arduino.h>
#include "arduinoFFT.h"
#include "pico/stdlib.h"
#include "hardware/dma.h"

#include "error.h"
#include "fix.h"

/* CONSTANTS */
#define NUM_OCTAVES 8
#define ROLLING_ITEMS 8

/* EXPORTED FUNTIONS */
void fft_init(fix15 *, uint16_t, uint32_t);
fix15 do_fft();
void freq2note(fix15 freq, uint8_t *note_index, int8_t *cents_deviation);
fix15 bin2freq(uint16_t bin);