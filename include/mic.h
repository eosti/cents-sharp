#pragma once
#include <Arduino.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

#include "error.h"

/* CONSTANTS */
#define MIC_ADC_PIN 26
#define CAPTURE_BITS 12
#define CAPTURE_DEPTH 1 << CAPTURE_BITS

/* EXPORTED FUNCTIONS */
void mic_init(uint16_t *, uint16_t *);
void mic_dma_handler(uint16_t *);
