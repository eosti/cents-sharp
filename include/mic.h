#pragma once
#include "error.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include <Arduino.h>

/* CONSTANTS */
#define MIC_ADC_PIN     26
#define CAPTURE_BITS    12
#define CAPTURE_DEPTH   1 << CAPTURE_BITS
#define MIC_SAMPLE_RATE 96000 * 2

static_assert(CAPTURE_BITS < 16, "CAPTURE_DEPTH must be less than 16 bits long");

/* MACROS */
#define ADC2VOLTAGE(a) ((a)*3.3f / (1 << 12))

/* EXPORTED FUNCTIONS */
void mic_init(uint16_t *front_buf, uint16_t *back_buf);
void mic_dma_handler(uint16_t *data);
