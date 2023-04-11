#include "display.h"

// Private prototypes
const void __note2char(display_note_t note, char *output);

U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0);

display_mode_t cur_mode = DISPLAY_INIT;
uint8_t DEVIATION_LUT[101];

void display_init() {
    // Reassign SDA/SCL
    Wire.setSDA(DISPLAY_SDA);
    Wire.setSCL(DISPLAY_SCL);

    if (!display.begin()) {
        fatal_error("Failed to initialize display");
    }
    display.enableUTF8Print();

    display.clearBuffer();
    display.setFont(u8g2_font_inr24_mf);
    
    // Draw a splash screen
    display.drawStr(40, 24, "\xA2""#");

    display.setFont(u8g2_font_fub11_tr);
    display.drawStr(14, 40, "Cents Sharp");

    display.setFont(u8g2_font_fur11_tr);
    display.drawStr(24, 58, "Initializing...");

    display.sendBuffer();

    // Generate LUT for the tuner
    for(int8_t i = -50; i <= 50; i++) {
        DEVIATION_LUT[i + 50] = DEVIATION_BOTTOM - DEVIATION_HEIGHT * sqrt(1 - pow((float(i)/DEVIATION_WIDTH), 2));
    }

    return;
}

void display_tuner(struct display_tuner_t *tuner) {
    cur_mode = DISPLAY_TUNER;

    display.clearBuffer();
    
    // Draw center frequency
    char center_frequency[8];
    sprintf(center_frequency, "%03d", tuner->center_frequency);
    display.setFont(u8g2_font_fub11_tr);
    display.drawStr(0, 64, center_frequency);

    // Draw tuner glyph
    display.setFont(u8g2_font_streamline_music_audio_t);
    display.drawStr(106, 64, "\x31");

    // Draw the tuner visualizer
#ifdef CIRCULAR_TUNER
    // We are given the percent deviation, so we can back-calculate the angle using math
    display.drawLine(64, 64, (tuner->cents_deviation)+64, DEVIATION_LUT[tuner->cents_deviation+50]);
#endif
#ifdef TRIANGLE_TUNER
    // Draw center box

    display.setDrawColor(1);
    display.drawBox(display.getWidth() / 2 - TRIANGLE_CENTER_OFFSET, TRIANGLE_CENTER_VERT_CENT - TRIANGLE_CENTER_HEIGHT / 2, TRIANGLE_CENTER_OFFSET * 2, TRIANGLE_CENTER_HEIGHT);

    // Draw triangle according to sharp/flat
    if(tuner->cents_deviation > -INTUNE_TOLERANCE && tuner->cents_deviation < INTUNE_TOLERANCE) {
        // perfectly in tune, nice
        display.setDrawColor(2);
        display.drawBox(0, 0, display.getWidth(), display.getHeight());
        display.setDrawColor(1);
    } else if (tuner->cents_deviation > 0) {
        // Sharp
        display.drawTriangle(
                                display.getWidth() / 2 + TRIANGLE_CENTER_OFFSET, 
                                TRIANGLE_CENTER_VERT_CENT - TRIANGLE_CENTER_HEIGHT / 2,
                                display.getWidth() / 2 + TRIANGLE_CENTER_OFFSET, 
                                TRIANGLE_CENTER_VERT_CENT + TRIANGLE_CENTER_HEIGHT / 2,
                                display.getWidth() / 2 + TRIANGLE_LENGTH,
                                TRIANGLE_CENTER_VERT_CENT
                            );
        display.setDrawColor(0);
        display.drawBox(display.getWidth() / 2 + tuner->cents_deviation, 0, 80, TRIANGLE_CENTER_HEIGHT / 2 + TRIANGLE_CENTER_VERT_CENT);
        display.setDrawColor(1);
    } else {
        // Flat
        display.drawTriangle(
                                display.getWidth() / 2 - TRIANGLE_CENTER_OFFSET, 
                                TRIANGLE_CENTER_VERT_CENT - TRIANGLE_CENTER_HEIGHT / 2,
                                display.getWidth() / 2 - TRIANGLE_CENTER_OFFSET, 
                                TRIANGLE_CENTER_VERT_CENT + TRIANGLE_CENTER_HEIGHT / 2,
                                display.getWidth() / 2 - TRIANGLE_LENGTH,
                                TRIANGLE_CENTER_VERT_CENT
                            );
        display.setDrawColor(0);
        display.drawBox(0, 0, display.getWidth() / 2 + tuner->cents_deviation, TRIANGLE_CENTER_HEIGHT / 2 + TRIANGLE_CENTER_VERT_CENT);
        display.setDrawColor(1);
    }


#endif
#ifdef BAR_TUNER
    // Draw center line
    display.setDrawColor(1);
    display.drawBox(display.getWidth() / 2 - BAR_CENTER_WIDTH / 2, BAR_CENTER_YPOS - BAR_CENTER_HEIGHT/ 2, BAR_CENTER_WIDTH, BAR_CENTER_HEIGHT);

    // Draw the tuning line
    if(tuner->cents_deviation > -INTUNE_TOLERANCE && tuner->cents_deviation < INTUNE_TOLERANCE) {
        // perfectly in tune, nice
        display.setDrawColor(2);
        display.drawBox(0, 0, display.getWidth(), display.getHeight());
        display.setDrawColor(1);
    } else if (tuner->cents_deviation > 0) {
        // Sharp
        display.drawBox(display.getWidth() / 2 + BAR_CENTER_WIDTH / 2, BAR_CENTER_YPOS - BAR_TUNING_HEIGHT / 2, 100, BAR_TUNING_HEIGHT);
        display.setDrawColor(0);
        display.drawBox(display.getWidth() / 2 + tuner->cents_deviation, 0, 100, BAR_CENTER_YPOS + BAR_TUNING_HEIGHT / 2);
        display.setDrawColor(1);
    } else {
        // Flat
        display.drawBox(0, BAR_CENTER_YPOS - BAR_TUNING_HEIGHT / 2, display.getWidth() / 2 - BAR_CENTER_WIDTH / 2, BAR_TUNING_HEIGHT);
        display.setDrawColor(0);
        display.drawBox(0, 0, display.getWidth() / 2 + tuner->cents_deviation, BAR_CENTER_YPOS + BAR_TUNING_HEIGHT / 2);
        display.setDrawColor(1);
    }

    // Draw left endcap
    display.drawVLine(display.getWidth() / 2 - 50 - BAR_ENDCAP_HOFFSET, BAR_CENTER_YPOS - BAR_ENDCAP_HEIGHT / 2, BAR_ENDCAP_HEIGHT);
    display.drawHLine(display.getWidth() / 2 - 50 - BAR_ENDCAP_HOFFSET, BAR_CENTER_YPOS + BAR_ENDCAP_HEIGHT / 2, BAR_ENDCAP_TIPLEN);
    display.drawHLine(display.getWidth() / 2 - 50 - BAR_ENDCAP_HOFFSET, BAR_CENTER_YPOS - BAR_ENDCAP_HEIGHT / 2, BAR_ENDCAP_TIPLEN);

    // Draw right endcap
    display.drawVLine(display.getWidth() / 2 + 50 + BAR_ENDCAP_HOFFSET, BAR_CENTER_YPOS - BAR_ENDCAP_HEIGHT / 2, BAR_ENDCAP_HEIGHT);
    display.drawHLine(display.getWidth() / 2 + 50 + BAR_ENDCAP_HOFFSET - BAR_ENDCAP_TIPLEN + 1, BAR_CENTER_YPOS + BAR_ENDCAP_HEIGHT / 2, BAR_ENDCAP_TIPLEN);
    display.drawHLine(display.getWidth() / 2 + 50 + BAR_ENDCAP_HOFFSET - BAR_ENDCAP_TIPLEN + 1, BAR_CENTER_YPOS - BAR_ENDCAP_HEIGHT / 2, BAR_ENDCAP_TIPLEN);

    // Draw markers (25%, 38%)
    display.drawVLine(display.getWidth() / 2 + 25, BAR_CENTER_YPOS, BAR_MARKER_LEN);
    display.drawVLine(display.getWidth() / 2 + 38, BAR_CENTER_YPOS, BAR_MARKER_LEN);
    display.drawVLine(display.getWidth() / 2 - 25, BAR_CENTER_YPOS, BAR_MARKER_LEN);
    display.drawVLine(display.getWidth() / 2 - 38, BAR_CENTER_YPOS, BAR_MARKER_LEN);

    // Draw double target marker
    display.drawVLine(display.getWidth() / 2 + 12, BAR_CENTER_YPOS - BAR_MARKER_LEN, BAR_MARKER_LEN * 2);
    display.drawVLine(display.getWidth() / 2 - 12, BAR_CENTER_YPOS - BAR_MARKER_LEN, BAR_MARKER_LEN * 2);
#endif

    // Draw the note
    char note[3] = "";
    __note2char(tuner->current_note, note);

    display.setFont(u8g2_font_inr24_mf);
    uint8_t w = display.getStrWidth(note);
    uint8_t h = display.getAscent() - display.getDescent();
    uint8_t x = 64 - w/2;
    uint8_t y = 64 - 4;

    if (strlen(note) == 0) {
        // Do nothing, no note
    } else if (strlen(note) == 2) {
// TODO: make a nice flat/sharp glyph
        if (note[1] == '#') {
            //draw sharp
            display.drawStr(x, y, note);
        } else {
            // draw flat
            display.drawStr(x, y, note);
        }
    } else {
        // Draw standard box, fill with note
        display.drawStr(x, y, note);
    }
    display.sendBuffer();
}

const void __note2char(const display_note_t note, char *output) {
    char msg[64];
    switch(note) {
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