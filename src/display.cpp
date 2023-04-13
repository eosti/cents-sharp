#include "display.h"

// Private prototypes
const void __note2char(display_note_t note, char *output);

// Global variables
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0);

#ifdef CIRCULAR_TUNER
uint8_t DEVIATION_LUT[101];
#endif

/**
 * @brief Inits display and sends a splash screen
 *
 */
void display_init() {
    // Reassign SDA/SCL
    Wire.setSDA(DISPLAY_SDA);
    Wire.setSCL(DISPLAY_SCL);

    if (!display.begin()) { fatal_error("Failed to initialize display"); }
    display.enableUTF8Print();

    display.clearBuffer();
    display.setFont(u8g2_font_inr24_mf);

    // Draw a splash screen
    display.drawStr(40, 24, "\xA2#");

    display.setFont(u8g2_font_fub11_tr);
    display.drawStr(14, 40, "Cents Sharp");

    display.setFont(u8g2_font_fur11_tr);
    display.drawStr(24, 58, "Initializing...");

    display.sendBuffer();

#ifdef CIRCULAR_TUNER
    // Generate LUT for the tuner
    for (int8_t i = -50; i <= 50; i++) {
        DEVIATION_LUT[i + 50] = DEVIATION_BOTTOM - DEVIATION_HEIGHT * sqrt(1 - pow((float(i) / DEVIATION_WIDTH), 2));
    }
#endif

    return;
}

/**
 * @brief Displays the tuner screen
 *
 * @param tuner parameters for the tuner
 * @note Multiple options available with #define [CIRCULAR_TUNER, TRIANGLE_TUNER, BAR_TUNER]
 */
void display_tuner(struct display_tuner_t *tuner) {
    display.clearBuffer();

    if (!(tuner->display_meme)) {
        // Draw center frequency
        char center_frequency[8];
        sprintf(center_frequency, "%03d", tuner->center_frequency);
        display.setFont(u8g2_font_fub11_tr);
        display.drawStr(0, 63, center_frequency);
    } else {
        // Draw currency conversion
        char cents_sharp[8];
        if (tuner->currency == CURRENCY_USD) {
            sprintf(cents_sharp, "$%0.02f", tuner->cents_deviation * 0.01);
        } else if (tuner->currency == CURRENCY_CAD) {
            sprintf(cents_sharp, "C$%0.02f", USD2CAD(tuner->cents_deviation));
        } else if (tuner->currency == CURRENCY_GBP) {
            sprintf(cents_sharp, "\xa3%0.02f", USD2GBP(tuner->cents_deviation));
        } else if (tuner->currency == CURRENCY_BTC) {
            sprintf(cents_sharp, "B%5d", USD2BTC(tuner->cents_deviation));
        } else if (tuner->currency == CURRENCY_YEN) {
            sprintf(cents_sharp, "\xa5%3.f", USD2YEN(tuner->cents_deviation));
        } else {
            sprintf(cents_sharp, "\xa4");
        }
        display.setFont(u8g2_font_fub11_tf);
        display.drawStr(0, 63, cents_sharp);
    }

    // Draw tuner glyph
    if (tuner->display_meme) {
        display.setFont(u8g2_font_streamline_money_payments_t);
        display.drawStr(106, 64, "\x38");
    } else {
        display.setFont(u8g2_font_streamline_music_audio_t);
        display.drawStr(106, 64, "\x31");
    }

    // if (tuner->low_noise) {
    //     display.setFont(u8g2_font_streamline_users_t);
    //     display.drawStr(54, 10, "\x3a");
    //     return;
    // }

    // Draw the tuner visualizer
#ifdef CIRCULAR_TUNER
    // We are given the percent deviation, so we can back-calculate the angle using math
    display.drawLine(64, 64, (tuner->cents_deviation) + 64, DEVIATION_LUT[tuner->cents_deviation + 50]);
#endif
#ifdef TRIANGLE_TUNER
    // Draw center box

    display.setDrawColor(1);
    display.drawBox(display.getWidth() / 2 - TRIANGLE_CENTER_OFFSET,
                    TRIANGLE_CENTER_VERT_CENT - TRIANGLE_CENTER_HEIGHT / 2,
                    TRIANGLE_CENTER_OFFSET * 2,
                    TRIANGLE_CENTER_HEIGHT);

    // Draw triangle according to sharp/flat
    if (tuner->cents_deviation > -INTUNE_TOLERANCE && tuner->cents_deviation < INTUNE_TOLERANCE) {
        // perfectly in tune, nice
        display.setDrawColor(2);
        display.drawBox(0, 0, display.getWidth(), display.getHeight());
        display.setDrawColor(1);
    } else if (tuner->cents_deviation > 0) {
        // Sharp
        display.drawTriangle(display.getWidth() / 2 + TRIANGLE_CENTER_OFFSET,
                             TRIANGLE_CENTER_VERT_CENT - TRIANGLE_CENTER_HEIGHT / 2,
                             display.getWidth() / 2 + TRIANGLE_CENTER_OFFSET,
                             TRIANGLE_CENTER_VERT_CENT + TRIANGLE_CENTER_HEIGHT / 2,
                             display.getWidth() / 2 + TRIANGLE_LENGTH,
                             TRIANGLE_CENTER_VERT_CENT);
        display.setDrawColor(0);
        display.drawBox(display.getWidth() / 2 + tuner->cents_deviation,
                        0,
                        80,
                        TRIANGLE_CENTER_HEIGHT / 2 + TRIANGLE_CENTER_VERT_CENT);
        display.setDrawColor(1);
    } else {
        // Flat
        display.drawTriangle(display.getWidth() / 2 - TRIANGLE_CENTER_OFFSET,
                             TRIANGLE_CENTER_VERT_CENT - TRIANGLE_CENTER_HEIGHT / 2,
                             display.getWidth() / 2 - TRIANGLE_CENTER_OFFSET,
                             TRIANGLE_CENTER_VERT_CENT + TRIANGLE_CENTER_HEIGHT / 2,
                             display.getWidth() / 2 - TRIANGLE_LENGTH,
                             TRIANGLE_CENTER_VERT_CENT);
        display.setDrawColor(0);
        display.drawBox(0,
                        0,
                        display.getWidth() / 2 + tuner->cents_deviation,
                        TRIANGLE_CENTER_HEIGHT / 2 + TRIANGLE_CENTER_VERT_CENT);
        display.setDrawColor(1);
    }

#endif
#ifdef BAR_TUNER
    // Draw center line
    display.setDrawColor(1);
    display.drawBox(display.getWidth() / 2 - BAR_CENTER_WIDTH / 2,
                    BAR_CENTER_YPOS - BAR_CENTER_HEIGHT / 2,
                    BAR_CENTER_WIDTH,
                    BAR_CENTER_HEIGHT);

    // Draw left endcap
    display.drawVLine(display.getWidth() / 2 - 50 - BAR_ENDCAP_HOFFSET,
                      BAR_CENTER_YPOS - BAR_ENDCAP_HEIGHT / 2,
                      BAR_ENDCAP_HEIGHT);
    display.drawHLine(display.getWidth() / 2 - 50 - BAR_ENDCAP_HOFFSET,
                      BAR_CENTER_YPOS + BAR_ENDCAP_HEIGHT / 2,
                      BAR_ENDCAP_TIPLEN);
    display.drawHLine(display.getWidth() / 2 - 50 - BAR_ENDCAP_HOFFSET,
                      BAR_CENTER_YPOS - BAR_ENDCAP_HEIGHT / 2,
                      BAR_ENDCAP_TIPLEN);

    // Draw right endcap
    display.drawVLine(display.getWidth() / 2 + 50 + BAR_ENDCAP_HOFFSET,
                      BAR_CENTER_YPOS - BAR_ENDCAP_HEIGHT / 2,
                      BAR_ENDCAP_HEIGHT);
    display.drawHLine(display.getWidth() / 2 + 50 + BAR_ENDCAP_HOFFSET - BAR_ENDCAP_TIPLEN + 1,
                      BAR_CENTER_YPOS + BAR_ENDCAP_HEIGHT / 2,
                      BAR_ENDCAP_TIPLEN);
    display.drawHLine(display.getWidth() / 2 + 50 + BAR_ENDCAP_HOFFSET - BAR_ENDCAP_TIPLEN + 1,
                      BAR_CENTER_YPOS - BAR_ENDCAP_HEIGHT / 2,
                      BAR_ENDCAP_TIPLEN);

    // Draw markers (25%, 38%)
    display.drawVLine(display.getWidth() / 2 + 25, BAR_CENTER_YPOS, BAR_MARKER_LEN);
    display.drawVLine(display.getWidth() / 2 + 38, BAR_CENTER_YPOS, BAR_MARKER_LEN);
    display.drawVLine(display.getWidth() / 2 - 25, BAR_CENTER_YPOS, BAR_MARKER_LEN);
    display.drawVLine(display.getWidth() / 2 - 38, BAR_CENTER_YPOS, BAR_MARKER_LEN);

    // Draw double target marker
    display.drawVLine(display.getWidth() / 2 + 12, BAR_CENTER_YPOS - BAR_MARKER_LEN, BAR_MARKER_LEN * 2);
    display.drawVLine(display.getWidth() / 2 - 12, BAR_CENTER_YPOS - BAR_MARKER_LEN, BAR_MARKER_LEN * 2);

    // Draw the tuning line
    if (tuner->cents_deviation > -INTUNE_TOLERANCE && tuner->cents_deviation < INTUNE_TOLERANCE) {
        // perfectly in tune, nice
        display.setDrawColor(2);
        display.drawBox(2, 2, display.getWidth() - 2, BAR_CENTER_HEIGHT + BAR_CENTER_YPOS - 2);
        display.setDrawColor(1);
    } else if (tuner->cents_deviation > 0) {
        // Sharp
        display.drawBox(display.getWidth() / 2 + BAR_CENTER_WIDTH / 2,
                        BAR_CENTER_YPOS - BAR_TUNING_HEIGHT / 2,
                        tuner->cents_deviation,
                        BAR_TUNING_HEIGHT);
    } else {
        // Flat
        display.drawBox(display.getWidth() / 2 + tuner->cents_deviation,
                        BAR_CENTER_YPOS - BAR_TUNING_HEIGHT / 2,
                        abs(tuner->cents_deviation),
                        BAR_TUNING_HEIGHT);
    }
#endif

    // Draw the note
    char note[3] = "";
    __note2char(tuner->current_note, note);

    display.setFont(u8g2_font_inr24_mf);
    uint8_t w = display.getStrWidth(note);
    uint8_t h = display.getAscent() - display.getDescent();
    uint8_t x = 64 - w / 2;
    if (tuner->display_meme) { x = 64 - w / 2 + 16; }
    uint8_t y = 64 - 4;

    if (strlen(note) == 0) {
        // Do nothing, no note
    } else if (strlen(note) == 2) {
        // TODO: make a nice flat/sharp glyph
        if (note[1] == '#') {
            // draw sharp
            display.drawStr(x, y, note);
        } else {
            // draw flat
            display.drawStr(x, y, note);
        }
    } else {
        // No sharp/flat
        display.drawStr(x, y, note);
    }
    display.sendBuffer();
}

/**
 * @brief displays a metronome
 *
 * @param tuner parameters for metronome
 */
void display_metronome(struct display_tuner_t *tuner) {
    display.clearBuffer();

    // Draw current BPM
    char cur_bpm[8];
    sprintf(cur_bpm, "%d", tuner->metronome_bpm);
    display.setFont(u8g2_font_inr38_mf);
    uint8_t w = display.getStrWidth(cur_bpm);
    uint8_t h = display.getAscent() - display.getDescent();
    uint8_t x = 64 - w / 2;
    uint8_t y = h - 6;
    display.drawStr(x, y, cur_bpm);

    // Draw metronome glyph
    display.setFont(u8g2_font_streamline_interface_essential_alert_t);
    display.drawStr(106, 64, "\x34");

    // Draw beats counter
    if (tuner->beat <= BEAT_9 && tuner->beat >= BEAT_0) {
        display.setFont(u8g2_font_unifont_t_72_73);
        if (tuner->beat == BEAT_0) {
            display.drawGlyph(1, 63, 0x24ea);
        } else {
            display.drawGlyph(1, 63, tuner->beat + 0x2460 - 2);
        }
        display.setFont(u8g2_font_unifont_t_76);
        display.drawGlyph(18, 63, 0x2669);
    } else if (tuner->beat == BEAT_NONE) {
        display.setFont(u8g2_font_unifont_h_symbols);
        display.drawGlyph(1, 63, 0x2715);
    } else if (tuner->beat == BEAT_EIGHTH) {
        display.setFont(u8g2_font_unifont_t_76);
        display.drawGlyph(1, 63, 0x266b);
    } else if (tuner->beat == BEAT_TRIPLET) {
        display.setFont(u8g2_font_unifont_t_76);
        display.drawGlyph(1, 63, 0x266a);
        display.drawGlyph(9, 63, 0x266a);
        display.drawGlyph(17, 63, 0x266a);
    } else if (tuner->beat == BEAT_SPLIT_TRIPLET) {
        display.setFont(u8g2_font_unifont_t_76);
        display.drawGlyph(1, 63, 0x266a);
        display.drawGlyph(9, 63, 0x2652);
        display.drawGlyph(17, 63, 0x266a);
    } else if (tuner->beat == BEAT_SIXTEEN) {
        display.setFont(u8g2_font_unifont_t_76);
        display.drawGlyph(1, 63, 0x266c);
        display.drawGlyph(9, 63, 0x266c);
    } else if (tuner->beat == BEAT_SPLIT_SIXTEEN) {
        display.setFont(u8g2_font_unifont_t_76);
        display.drawGlyph(1, 63, 0x266a);
        display.drawGlyph(9, 63, 0x2652);
        display.drawGlyph(17, 63, 0x2652);
        display.drawGlyph(25, 63, 0x266a);
    } else {
        display.setFont(u8g2_font_unifont_h_symbols);
        display.setCursor(1, 63);
        display.print("\x3F\x3F");
    }
    display.sendBuffer();
}

/**
 * @brief Converts a enum note into a char array
 *
 * @param note enum note
 * @param output output char array
 */
const void __note2char(const display_note_t note, char *output) {
    char msg[64];
    switch (note) {
        case NOTE_A_FLAT:
            strncpy(output, "Ab", 2);
            break;
        case NOTE_A:
            strncpy(output, "A", 1);
            break;
        case NOTE_B_FLAT:
            strncpy(output, "Bb", 2);
            break;
        case NOTE_B:
            strncpy(output, "B", 1);
            break;
        case NOTE_C:
            strncpy(output, "C", 1);
            break;
        case NOTE_C_SHARP:
            strncpy(output, "C#", 2);
            break;
        case NOTE_D:
            strncpy(output, "D", 1);
            break;
        case NOTE_E_FLAT:
            strncpy(output, "Eb", 2);
            break;
        case NOTE_E:
            strncpy(output, "E", 1);
            break;
        case NOTE_F:
            strncpy(output, "F", 1);
            break;
        case NOTE_F_SHARP:
            strncpy(output, "F#", 2);
            break;
        case NOTE_G:
            strncpy(output, "G", 1);
            break;
        case NOTE_NONE:
        default:
            strncpy(output, "", 0);
            break;
    }
}