#pragma once
#include <Arduino.h>
#include "pico/stdlib.h"
#include "hardware/exception.h"

#include "fix.h"

/* Types */
enum error_t {
    DEBUG,
    INFO, 
    WARNING,
    ERROR
};

/* CONSTANTS */
#define MSG_LEVEL DEBUG

/* EXPORTED FUNCTIONS */
void error_init();
void fatal_error(const char*);
void print_msg(const char*, error_t);
void dump_array_uint16(uint16_t *array, uint16_t len, const char *msg);
void dump_array_fix15(fix15 *array, uint16_t len, const char *msg);
void dump_array_double(double *array, uint16_t len, const char *msg);