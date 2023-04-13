#include "fft.h"

// Private defs
const void __populate_freq_lut(uint16_t tune_a);
const fix15 __average(fix15 *, uint8_t count);
const fix15 __variance(fix15 *, uint8_t count, fix15 avg);
const fix15 __median(fix15 *arr, uint8_t count);

// Global variables
uint16_t *data_input                 = NULL;
fix15 *data_output                   = NULL;
fix15 *Sinewave                      = NULL;
uint fft_dma_channel                 = 0;
uint16_t CAPTURE_DEPTH               = 0;
uint16_t CAPTURE_BITS                = 0;
uint32_t SAMPLE_RATE                 = 0;
uint16_t tune_a_value                = 0;
double FREQ2OCTAVE_CONSTANT          = 0;
double LOG_1_12_BASE                 = 0;

fix15 rolling_buffer[ROLLING_ITEMS]  = {0};
fix15 rolling_average                = 0;
fix15 rolling_deviance               = 0;
uint8_t rolling_index                = 0;
uint8_t rolling_outlier_count        = 0;
fix15 rolling_outlier[ROLLING_ITEMS] = {0};

double FREQ_LUT[12 * NUM_OCTAVES]    = {0};

/**
 * @brief initializes the FFT functionality
 *
 * @param fft_output fix15 array to output FFT data to
 * @param num_bits equal to CAPTURE_BITS
 */
void fft_init(fix15 *fft_output, uint16_t num_bits, uint32_t samplerate) {
    data_output   = fft_output;
    CAPTURE_BITS  = num_bits;
    CAPTURE_DEPTH = 1 << num_bits;
    SAMPLE_RATE   = samplerate;

    char msg[64];
    sprintf(msg, "FFT inited with %d depth (%d bits)", CAPTURE_DEPTH, CAPTURE_BITS);
    print_msg(msg, INFO);

    // Sinewave generation
    Sinewave = (fix15 *)malloc(sizeof(fix15) * CAPTURE_DEPTH);
    for (uint16_t i = 0; i < CAPTURE_DEPTH; i++) {
        Sinewave[i] = float2fix15(sin(6.283 * ((float)i / CAPTURE_DEPTH)));
    }

    // DMA config to transfer raw data into a FFT buffer
    fft_dma_channel        = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(fft_dma_channel);

    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32); // Use 32-bit transfers
    channel_config_set_read_increment(&cfg, true);            // Read from incrementing address
    channel_config_set_write_increment(&cfg, true);           // Write to an incrementing address

    dma_channel_configure(fft_dma_channel,
                          &cfg,
                          data_output,       // destination buffer
                          data_input,        // source FIFO
                          CAPTURE_DEPTH / 2, // transfer count
                          false              // don't start yet
    );

    FREQ2OCTAVE_CONSTANT = pow(2, (double)1 / 12);
    LOG_1_12_BASE        = 1 / log((double)1 / 12);
    __populate_freq_lut(tune_a_value);
}

void mic_dma_handler(uint16_t *data) {
    // This will run each time the DMA completes, so keep it snappy!
    data_input = data;
}

/**
 * @brief Computes the FFT of data_input into data_output
 *
 * @param data_output fix15 array to store result
 * @return uint16_t index of max value, 0 if invalid
 */
fix15 do_fft() {
    if (data_input == NULL) {
        // No pending data, so return
        return 0;
    }

    char msg[64];
    // dump_array_uint16(data_input, 64, "do_fft input:");

    // Expected input is an CAPTURE_DEPTH length array of 12-bit readings
    // Copy 16-bit data into space that's zero padded to be 32-bit
    // then copy that 32-bit into array
    // We want to move this into a seperate buffer
    // dma_channel_set_read_addr(fft_dma_channel, data_input, false);
    // dma_channel_set_write_addr(fft_dma_channel, data_output, true);
    // dma_channel_start(fft_dma_channel);
    // do we need to wait for DMA to start?

    // Error checking: error flag stored in bit 15 of the ADC data
    uint16_t data_error = 0;
    // this loop is slower than the DMA, so shouldn't have a race condition
    for (uint16_t i = 0; i < CAPTURE_DEPTH; i++) {
        if (data_input[i] & 0x8000) {
            // Clear error bit for processing
            data_error++;
            data_input[i] &= 0x7FFF;
        }
        data_output[i] = int2fix15(data_input[i]);
        // uint16_t amplitude = 400;
        // uint16_t signalFrequency = 2000;
        // float cycles = (((CAPTURE_DEPTH-1) * signalFrequency) / SAMPLE_RATE);
        // data_output[i] = float2fix15((amplitude * (sin((i * (twoPi * cycles)) / CAPTURE_DEPTH))) / 2.0 + amplitude);
    }

    if (data_error) {
        sprintf(msg, "%d ADC errors detected out of %d samples", data_error, CAPTURE_DEPTH);
        print_msg(msg, WARNING);
    }

    // transfer complete, so we reset the input buffer
    data_input = NULL;

    // Step 0: windowing wising Hann function
    for (uint16_t i = 0; i < CAPTURE_DEPTH; i++) {
        fix15 multiplier = (int2fix15(1) - Sinewave[abs(i - CAPTURE_DEPTH / 4) & 0xFFFF]) >> 1;
        data_output[i]   = multiply_fix15(multiplier, data_output[i]);
    }

    // dump_array_fix15(data_output, 64, "do_fft transfer");
    // dump_array_double(vReal, 64, "do_fft transfer");

    /* FFT Algo adapted from https://vanhunteradams.com/FFT/FFT.html */

    // Step 1: bit reversal
    // Here, we reverse the order of the bits of the indices
    for (uint16_t i = 1; i < CAPTURE_DEPTH - 1; i++) {
        // We can skip the first and last indices because 0x0000 and 0xFFFF flipped is just itself
        // Bit reversal from https://graphics.stanford.edu/~seander/bithacks.html#ReverseParallel
        uint16_t v = i; // 16-bit word to reverse bit order

        // swap odd and even bits
        v = ((v >> 1) & 0x5555) | ((v & 0x5555) << 1);
        // swap consecutive pairs
        v = ((v >> 2) & 0x3333) | ((v & 0x3333) << 2);
        // swap nibbles ...
        v = ((v >> 4) & 0x0F0F) | ((v & 0x0F0F) << 4);
        // swap bytes
        v = ((v >> 8) & 0x00FF) | ((v & 0x00FF) << 8);
        // Adjust for total number of samples
        v >>= (16 - CAPTURE_BITS);

        // Don't swap what's already been swapped
        if (v <= i) continue;

        // Swap bit-reversed indices
        fix15 tmp      = data_output[i];
        data_output[i] = data_output[v];
        data_output[v] = tmp;
    }

    // dump_array_fix15(data_output, 64, "do_fft bitreversal");

    // Step 2: FFT (Danielson-Lanczos)
    fix15 imag_buf[CAPTURE_DEPTH] = {0};
    uint16_t fft_len              = 1;
    uint16_t fft_bits             = CAPTURE_BITS - 1;
    while (fft_len < CAPTURE_DEPTH) {
        // Determine new FFT length
        uint16_t new_fft_len = fft_len << 1;

        // Combine elements in FFTs
        for (uint16_t i = 0; i < fft_len; i++) {
            // Get trig values for this element (0.5*cos/sin(sample number))
            uint32_t bt    = i << fft_bits;
            fix15 sin_term = -Sinewave[bt];
            fix15 cos_term = Sinewave[(bt + CAPTURE_DEPTH / 4) & 0xFFFF];
            sin_term >>= 1;
            cos_term >>= 1;

            for (uint16_t k = i; k < CAPTURE_DEPTH; k += new_fft_len) {
                uint32_t bn     = k + fft_len;

                fix15 real      = multiply_fix15(cos_term, data_output[bn]) - multiply_fix15(sin_term, imag_buf[bn]);
                fix15 imag      = multiply_fix15(cos_term, imag_buf[bn]) + multiply_fix15(sin_term, data_output[bn]);

                fix15 real_tmp  = data_output[k] >> 1;
                fix15 imag_tmp  = imag_buf[k] >> 1;

                data_output[bn] = real_tmp - real;
                imag_buf[bn]    = imag_tmp - imag;
                data_output[k]  = real_tmp + real;
                imag_buf[k]     = imag_tmp + imag;
            }
        }
        fft_bits--;
        fft_len = new_fft_len;
    }

    // Step 3: Peak detection
    fix15 max_val  = 0;
    uint16_t i_max = 0;
    for (uint16_t i = 1; i < CAPTURE_DEPTH / 2; i++) {
        if ((data_output[i] > data_output[i - 1]) && data_output[i] > data_output[i + 1]) {
            if (data_output[i] > max_val) {
                max_val = data_output[i];
                i_max   = i;
            }
        }
    }

    sprintf(msg, "DC: %f, peak: %f", fix2float15(data_output[0]), fix2float15(max_val));
    print_msg(msg, DEBUG);

    // Step 3.5: Low-Noise Cutoff
    if (max_val < LOW_NOISE_THRESH) { return int2fix15(-1); }

    // Step 4: Weighted interpolation
    fix15 delta = (data_output[i_max - 1] - data_output[i_max + 1]) >> 1;
    delta       = divide_fix15(delta, (data_output[i_max - 1] + data_output[i_max + 1] - 2 * data_output[i_max]));
    fix15 interpolated = multiply_fix15((int2fix15(i_max) + delta), float2fix15((float)SAMPLE_RATE / CAPTURE_DEPTH));

    // sprintf(msg, "Original: %f Interpolated: %f", fix2float15(data_output[i_max]), fix2float15(interpolated));
    // print_msg(msg, DEBUG);

    // Step 5: rolling average w/ outlier detection
    if (interpolated > rolling_average + rolling_deviance || interpolated < rolling_average - rolling_deviance) {
        if (rolling_outlier_count >= ROLLING_OUTLIER_THRESH) {
            // Swap in array
            print_msg("rolling average: swap buffers", INFO);
            memcpy(rolling_buffer, rolling_outlier, ROLLING_ITEMS * sizeof(fix15));
            rolling_index         = ROLLING_OUTLIER_THRESH;
            rolling_outlier_count = 0;
        } else {
            // This is an outlier, so shelve it
            rolling_outlier[rolling_outlier_count] = interpolated;
            rolling_outlier_count++;
            sprintf(msg,
                    "Skipped frequency of %f (mean=%f, var=%f)",
                    fix2float15(interpolated),
                    fix2float15(rolling_average),
                    fix2float15(rolling_deviance));
            // print_msg(msg, DEBUG);
            return rolling_average;
        }
    }

    // Otherwise, add to buffer
    rolling_outlier_count = 0;
    sprintf(msg,
            "Added frequency of %f (mean=%f, var=%f)",
            fix2float15(interpolated),
            fix2float15(rolling_average),
            fix2float15(rolling_deviance));
    rolling_buffer[rolling_index] = interpolated;
    rolling_average               = __average(rolling_buffer, ROLLING_ITEMS);
    rolling_deviance              = multiply_fix15(float2fix15(ROLLING_DEVIANCE_MULT), rolling_average);
    // rolling_variance = __variance(rolling_buffer, ROLLING_ITEMS, rolling_average);

    // rolling_buffer[rolling_index] = interpolated;
    // rolling_average = __median(rolling_buffer, ROLLING_ITEMS);
    // sprintf(msg, "Median value is %f", fix2float15(rolling_average));
    // print_msg(msg, DEBUG);

    if (rolling_index < ROLLING_ITEMS) {
        rolling_index++;
    } else {
        rolling_index = 0;
    }

    return rolling_average;
}

/**
 * @brief Generates a LUT based on a reference A4 frequency
 * @remarks Math based on https://pages.mtu.edu/~suits/NoteFreqCalcs.html
 *
 * @param tune_a Frequency to reference A4 to
 */
const void __populate_freq_lut(uint16_t tune_a) {
    for (uint8_t i = 0; i < 12 * NUM_OCTAVES; i++) {
        // We tune to A4, so C0 is 4*12 + 9 below that
        FREQ_LUT[i] = (double)tune_a * pow(FREQ2OCTAVE_CONSTANT, i - (4 * 12 + 9));
    }
}

/**
 * @brief Determines the closest note to the given frequency, plus the percent deviation
 *
 * @param freq input frequency
 * @param note_index number of half-steps above C0
 * @param cents_deviation percent deviation from closest note (max val is ±50)
 */
void freq2note(fix15 freq, uint8_t *note_index, int8_t *cents_deviation) {
    char msg[64];
    float input_float = fix2float15(freq);
    for (uint8_t i = 0; i < 12 * NUM_OCTAVES; i++) {
        if (input_float < FREQ_LUT[i]) {
            // We know this is the frequency right above the actual note
            // Convert it all to log scale
            double upper_value  = log(FREQ_LUT[i]) * FREQ2OCTAVE_CONSTANT;
            double lower_value  = log(FREQ_LUT[i - 1]) * FREQ2OCTAVE_CONSTANT;
            double target_value = log(input_float) * FREQ2OCTAVE_CONSTANT;
            double high_diff    = upper_value - target_value;
            double low_diff     = lower_value - target_value;

            if (high_diff > abs(low_diff)) {
                // It's closer to the lower note
                *note_index      = i - 1;
                *cents_deviation = (int)round(low_diff / (upper_value - lower_value) * 100);
            } else {
                *note_index      = i;
                *cents_deviation = (int)round(high_diff / (upper_value - lower_value) * 100);
            }
            return;
        }
    }
    return;
}

const fix15 __average(fix15 *buf, uint8_t count) {
    fix15 sum    = 0;
    uint8_t omit = 0;
    for (int i = 0; i < count; i++) {
        if (buf[i] == 0) {
            omit++;
            continue;
        } else {
            sum += buf[i];
        }
    }
    return multiply_fix15(sum, float2fix15((float)1 / (count - omit)));
}

const fix15 __variance(fix15 *buf, uint8_t count, fix15 avg) {
    fix15 stddev = 0;
    uint8_t omit = 0;
    for (int i = 0; i < count; i++) {
        if (buf[i] == 0) {
            omit++;
            continue;
        } else {
            stddev += multiply_fix15((buf[i] - avg), (buf[i] - avg));
        }
    }
    return multiply_fix15(stddev, float2fix15((float)1 / (count - omit)));
}

const fix15 __median(fix15 *arr, uint8_t count) {
    fix15 tmp[ROLLING_ITEMS];
    memcpy(tmp, arr, ROLLING_ITEMS * sizeof(fix15));

    // Sort
    for (uint8_t i = 1; i <= count - 1; i++) {
        for (uint8_t j = 1; j <= count - i; j++) {
            if (arr[j] <= arr[j + 1]) {
                fix15 t    = tmp[j];
                tmp[j]     = tmp[j + 1];
                tmp[j + 1] = t;
            } else {
                continue;
            }
        }
    }

    if (count % 2 == 0) {
        return (tmp[count / 2] + tmp[count / 2 + 1]) >> 1;
    } else {
        return tmp[count / 2 + 1];
    }
}

/**
 * @brief Changes what frequency A5 is referred to
 *
 * @param new_center new center frequency
 */
void change_fft_center(uint16_t new_center) {
    tune_a_value = new_center;
    __populate_freq_lut(tune_a_value);
}