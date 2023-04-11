#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "error.h"

/* Types */
enum display_mode_t {
    DISPLAY_INIT,
    DISPLAY_TUNER,
    DISPLAY_TUNER_MEME,
    DISPLAY_METRO
};

enum display_note_t {
    NOTE_C,
    NOTE_C_SHARP,
    NOTE_D,
    NOTE_E_FLAT,
    NOTE_E,
    NOTE_F,
    NOTE_F_SHARP,
    NOTE_G,
    NOTE_A_FLAT,
    NOTE_A,
    NOTE_B_FLAT,
    NOTE_B,
    NOTE_NONE
};

typedef struct display_tuner_t {
    uint16_t center_frequency;
    display_note_t current_note;
    int8_t cents_deviation;
} tuner_t;

/* CONSTANTS */
#define DISPLAY_SDA 16
#define DISPLAY_SCL 17
#define DISPLAY_WIDTH 128 
#define DISPLAY_HEIGHT 64 
#define DISPLAY_ADDRESS 0x3C

// General tuner parameters
#define INTUNE_TOLERANCE 2 // ± 2%
#define INTUNE_MARKER 12

// Circular tuner parameters
#define DEVIATION_BOTTOM 50
#define DEVIATION_HEIGHT 48
#define DEVIATION_WIDTH 50 // don't change this one

// Triangle tuner parameters
#define TRIANGLE_CENTER_OFFSET 1
#define TRIANGLE_CENTER_HEIGHT 26
#define TRIANGLE_CENTER_VERT_CENT 16
#define TRIANGLE_LENGTH 60

// Bar tuner parameters
#define BAR_CENTER_HEIGHT 16
#define BAR_CENTER_WIDTH 2
#define BAR_CENTER_YPOS 16
#define BAR_TUNING_HEIGHT 4
#define BAR_ENDCAP_HEIGHT 8
#define BAR_ENDCAP_HOFFSET 2
#define BAR_ENDCAP_TIPLEN 4
#define BAR_MARKER_LEN 4

//#define CIRCULAR_TUNER
// #define TRIANGLE_TUNER
#define BAR_TUNER

/* EXPORTED FUNCTIONS */
void display_init();
void display_tuner(struct display_tuner_t *tuner);