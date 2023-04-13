#pragma once
#include "fix.h"
#include "hardware/exception.h"
#include "pico/stdlib.h"

#include <Arduino.h>

/* TYPES */
enum error_t { DEBUG, INFO, WARNING, ERROR };

/* CONSTANTS */
#define MSG_LEVEL DEBUG

/* EXPORTED FUNCTIONS */
void error_init();
void fatal_error(const char *msg);
void print_msg(const char *msg, error_t msg_type);
void dump_array_uint16(uint16_t *array, uint16_t len, const char *msg);
void dump_array_fix15(fix15 *array, uint16_t len, const char *msg);
void dump_array_double(double *array, uint16_t len, const char *msg);