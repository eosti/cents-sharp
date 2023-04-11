#include "tuner.h"

uint16_t buffer1[CAPTURE_DEPTH];
uint16_t buffer2[CAPTURE_DEPTH];
fix15 fft_buffer[CAPTURE_DEPTH];

display_tuner_t tuner;

void setup() {
  error_init();
  delay(4000);
  print_msg("Beginning Setup", INFO);

  mic_init(buffer1, buffer2);
  fft_init(fft_buffer, CAPTURE_BITS);
  display_init();
  print_msg("Setup complete!", INFO);
  tuner.center_frequency = 440;
  
  // for(fix15 i = int2fix15(1); i < int2fix15(10000); i += int2fix15(1)) {
  //   char msg[64];
  //   uint8_t index;
  //   freq2note(i, &index, &(tuner.cents_deviation));
  //   tuner.current_note = (display_note_t)(index % 12);
  //   sprintf(msg, "Main current note: %d", index);
  //   print_msg(msg, DEBUG);
  //   display_tuner(&tuner);
  //   delay(10);
  // }

  // tuner.current_note = NOTE_A_FLAT;
  // for(int i = -50; i <= 50; i++) {
  //   tuner.cents_deviation = i;
  //   display_tuner(&tuner);
  //   delay(300);
  // }
}

void loop() {
  // put your main code here, to run repeatedly:
  double result = do_fft();
  if (result != 0) {
    // fix15 freq = bin2freq(result);

    char msg[64];
    // sprintf(msg, "Primary frequency is %f", fix2float15(freq));
    sprintf(msg, "Primary frequency is %f", result);
    print_msg(msg, INFO);

    uint8_t index;
    freq2note(float2fix15(result), &index, &(tuner.cents_deviation));
    tuner.current_note = (display_note_t)(index % 12);
    display_tuner(&tuner);
  }
}