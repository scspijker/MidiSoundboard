#ifdef ESP_PLATFORM

#include "../audio.h"
#include "driver/i2s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <stdio.h>

#define I2S_NUM I2S_NUM_0
#define I2S_BCK_PIN GPIO_NUM_26
#define I2S_WS_PIN GPIO_NUM_25
#define I2S_DATA_PIN GPIO_NUM_22

static uint32_t sample_rate = 44100;
static bool initialized = false;
static QueueHandle_t audio_queue = NULL;

int audio_init(uint32_t sr) {
    if (initialized) {
        return 0;
    }
    
    sample_rate = sr;
    
    // Create queue for audio samples
    audio_queue = xQueueCreate(5, sizeof(size_t)); // Store sample count
    if (audio_queue == NULL) {
        return -1;
    }
    
    // Configure I2S
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN,
        .sample_rate = sample_rate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = true,
    };
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_DATA_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    
    esp_err_t err = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        vQueueDelete(audio_queue);
        return -1;
    }
    
    err = i2s_set_pin(I2S_NUM, &pin_config);
    if (err != ESP_OK) {
        i2s_driver_uninstall(I2S_NUM);
        vQueueDelete(audio_queue);
        return -1;
    }
    
    // Enable DAC
    i2s_set_dac_mode(I2S_DAC_CHANNEL_LEFT_EN);
    
    initialized = true;
    return 0;
}

int audio_play_sample(const int16_t *samples, size_t sample_count) {
    if (!initialized || samples == NULL || sample_count == 0) {
        return -1;
    }
    
    // Convert int16_t samples to uint8_t for DAC (8-bit)
    size_t bytes_written = 0;
    size_t bytes_to_write = sample_count * sizeof(int16_t);
    
    // I2S DAC expects 8-bit samples, so we need to convert
    // For simplicity, we'll use the high byte of each 16-bit sample
    uint8_t *dac_buffer = malloc(sample_count);
    if (dac_buffer == NULL) {
        return -1;
    }
    
    for (size_t i = 0; i < sample_count; i++) {
        // Convert signed 16-bit to unsigned 8-bit (DAC format)
        int16_t sample = samples[i];
        // Scale and offset to 0-255 range
        dac_buffer[i] = (uint8_t)((sample >> 8) + 128);
    }
    
    esp_err_t err = i2s_write(I2S_NUM, dac_buffer, sample_count, &bytes_written, portMAX_DELAY);
    free(dac_buffer);
    
    if (err != ESP_OK || bytes_written != sample_count) {
        return -1;
    }
    
    return 0;
}

void audio_cleanup(void) {
    if (!initialized) {
        return;
    }
    
    if (audio_queue != NULL) {
        vQueueDelete(audio_queue);
        audio_queue = NULL;
    }
    
    i2s_driver_uninstall(I2S_NUM);
    initialized = false;
}

#endif // ESP_PLATFORM
