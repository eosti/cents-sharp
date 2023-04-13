#pragma once
#include "error.h"

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

/* TYPES */
enum display_note_t {
    NOTE_A_FLAT,
    NOTE_A,
    NOTE_B_FLAT,
    NOTE_B,
    NOTE_C,
    NOTE_C_SHARP,
    NOTE_D,
    NOTE_E_FLAT,
    NOTE_E,
    NOTE_F,
    NOTE_F_SHARP,
    NOTE_G,
    NOTE_NONE
};

enum display_beat_t {
    BEAT_NONE,
    BEAT_0,
    BEAT_1,
    BEAT_2,
    BEAT_3,
    BEAT_4,
    BEAT_5,
    BEAT_6,
    BEAT_7,
    BEAT_8,
    BEAT_9,
    BEAT_EIGHTH,
    BEAT_TRIPLET,
    BEAT_SPLIT_TRIPLET,
    BEAT_SIXTEEN,
    BEAT_SPLIT_SIXTEEN
};

enum currency_t { CURRENCY_USD, CURRENCY_CAD, CURRENCY_YEN, CURRENCY_GBP, CURRENCY_BTC };

typedef struct display_tuner_t {
    uint16_t center_frequency;
    display_note_t current_note;
    int8_t cents_deviation;
    bool display_meme;
    uint16_t metronome_bpm;
    display_beat_t beat;
    uint8_t mode_sel;
    bool low_noise;
    currency_t currency;
    bool soundback_en;
    display_note_t soundback_note;
    uint8_t soundback_octave;
} tuner_t;

/* CONSTANTS */
#define DISPLAY_SDA     16
#define DISPLAY_SCL     17
#define DISPLAY_WIDTH   128
#define DISPLAY_HEIGHT  64
#define DISPLAY_ADDRESS 0x3C

// General tuner parameters
#define INTUNE_TOLERANCE 2 // ± 2%
#define INTUNE_MARKER    12

// Circular tuner parameters
#define DEVIATION_BOTTOM 50
#define DEVIATION_HEIGHT 48
#define DEVIATION_WIDTH  50 // don't change this one

// Triangle tuner parameters
#define TRIANGLE_CENTER_OFFSET    1
#define TRIANGLE_CENTER_HEIGHT    26
#define TRIANGLE_CENTER_VERT_CENT 16
#define TRIANGLE_LENGTH           60

// Bar tuner parameters
#define BAR_CENTER_HEIGHT  16
#define BAR_CENTER_WIDTH   2
#define BAR_CENTER_YPOS    16
#define BAR_TUNING_HEIGHT  4
#define BAR_ENDCAP_HEIGHT  8
#define BAR_ENDCAP_HOFFSET 2
#define BAR_ENDCAP_TIPLEN  4
#define BAR_MARKER_LEN     4

// #define CIRCULAR_TUNER
// #define TRIANGLE_TUNER
#define BAR_TUNER

/* MACROS */
#define USD2CAD(a) ((1.35 * (a)*0.01))
#define USD2GBP(a) ((0.8 * (a)*0.01))
#define USD2YEN(a) ((1.34 * (a)))
#define USD2BTC(a) (((a)*rand()) % 10000)

/* EXPORTED FUNCTIONS */
void display_init();
void display_tuner(struct display_tuner_t *tuner);
void display_metronome(struct display_tuner_t *tuner);
void display_soundback(struct display_tuner_t *tuner);