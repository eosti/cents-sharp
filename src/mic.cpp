#include "mic.h"

// Private functions
void __dma_0_handler();

// Globals
uint16_t *dma_front_buf = NULL;
uint16_t *dma_back_buf  = NULL;
uint mic_dma_channel    = 0;

/**
 * @brief Inits the ADC and DMA for the microphone
 *
 * @param front_buf front buffer for DMA transfer
 * @param back_buf back buffer for DMA transfer
 */
void mic_init(uint16_t *front_buf, uint16_t *back_buf) {
    dma_front_buf = front_buf;
    dma_back_buf  = back_buf;

    /* ADC CONFIG */
    adc_init();
    adc_gpio_init(MIC_ADC_PIN);
    adc_select_input(MIC_ADC_PIN - 26);         // sets ADC mux, 0-3 correspond to GPIO 26-29
    adc_set_clkdiv(48000000 / MIC_SAMPLE_RATE); // Base clock is 48MHz, we want to sample at 96KHz

    adc_fifo_setup(true, // Enable FIFO
                   true, // Enable DMA request
                   1,    // DMA request on every sample
                   true, // Bit 15 is error flag
                   false // Don't truncate to 8 bits
    );
    adc_fifo_drain();

    /* DMA CONFIG*/
    mic_dma_channel        = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(mic_dma_channel);

    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16); // Use 16-bit transfers
    channel_config_set_read_increment(&cfg, false);           // Read from same address
    channel_config_set_write_increment(&cfg, true);           // Write to an incrementing address
    channel_config_set_dreq(&cfg, DREQ_ADC);                  // Do a transfer when ADC FIFO sends DMA request

    dma_channel_configure(mic_dma_channel,
                          &cfg,
                          dma_front_buf, // destination buffer
                          &adc_hw->fifo, // source FIFO
                          CAPTURE_DEPTH, // transfer count
                          false          // don't start yet
    );

    // Set IRQ1 when transfer completed
    dma_channel_set_irq0_enabled(mic_dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, __dma_0_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    // start everything up now
    adc_run(true);
    dma_channel_start(mic_dma_channel);
}

/**
 * @brief Handler for IRQ0 DMA requests
 */
void __dma_0_handler() {
    if (!dma_channel_get_irq0_status(mic_dma_channel)) { fatal_error("Unknown DMA channel interrupt"); }

    dma_channel_acknowledge_irq0(mic_dma_channel);

    // Swap front and back buffers
    uint16_t *tmp = dma_front_buf;
    dma_front_buf = dma_back_buf;
    dma_back_buf  = tmp;

    mic_dma_handler(dma_back_buf);

    // Restart the dma
    dma_channel_set_write_addr(mic_dma_channel, dma_front_buf, false);
    dma_channel_start(mic_dma_channel);
}

/**
 * @brief Weakly defined handler for the mic DMA
 * @note adds ability for other places to take action on DMA transfer complete
 *
 */
__attribute__((weak)) void mic_dma_handler(uint16_t *data) {
    // noop if not defined
    print_msg("Mic handler not implemented", INFO);
}