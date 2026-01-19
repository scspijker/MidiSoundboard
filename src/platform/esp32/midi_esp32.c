#ifdef ESP_PLATFORM

#include "../midi.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <stdio.h>

#define UART_NUM UART_NUM_2
#define BUF_SIZE 1024
#define MIDI_BAUD_RATE 31250  // Standard MIDI baud rate

static QueueHandle_t midi_queue = NULL;
static TaskHandle_t midi_task_handle = NULL;
static bool initialized = false;

static void midi_task(void *pvParameters) {
    (void)pvParameters;
    uint8_t data[BUF_SIZE];
    midi_event_t event;
    uint8_t status_byte = 0;
    uint8_t note = 0;
    uint8_t velocity = 0;
    uint8_t bytes_received = 0;
    
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE, pdMS_TO_TICKS(100));
        
        for (int i = 0; i < len; i++) {
            uint8_t byte = data[i];
            
            // Check if it's a status byte (MSB is 1)
            if (byte & 0x80) {
                status_byte = byte;
                bytes_received = 0;
                
                // Note On or Note Off
                if ((status_byte & 0xF0) == 0x90 || (status_byte & 0xF0) == 0x80) {
                    bytes_received = 1;
                }
            } else {
                // Data byte
                if ((status_byte & 0xF0) == 0x90 || (status_byte & 0xF0) == 0x80) {
                    if (bytes_received == 1) {
                        note = byte;
                        bytes_received = 2;
                    } else if (bytes_received == 2) {
                        velocity = byte;
                        
                        event.note = note;
                        event.velocity = velocity;
                        event.is_on = ((status_byte & 0xF0) == 0x90) && (velocity > 0);
                        
                        xQueueSend(midi_queue, &event, 0);
                        bytes_received = 0;
                    }
                }
            }
        }
    }
}

int midi_init(void) {
    if (initialized) {
        return 0;
    }
    
    // Configure UART for MIDI
    uart_config_t uart_config = {
        .baud_rate = MIDI_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    // Create queue for MIDI events
    midi_queue = xQueueCreate(10, sizeof(midi_event_t));
    if (midi_queue == NULL) {
        uart_driver_delete(UART_NUM);
        return -1;
    }
    
    // Create MIDI reading task
    xTaskCreate(midi_task, "midi_task", 4096, NULL, 5, &midi_task_handle);
    if (midi_task_handle == NULL) {
        vQueueDelete(midi_queue);
        uart_driver_delete(UART_NUM);
        return -1;
    }
    
    initialized = true;
    return 0;
}

int midi_read(midi_event_t *event) {
    if (event == NULL || !initialized) {
        return -1;
    }
    
    if (xQueueReceive(midi_queue, event, pdMS_TO_TICKS(10)) == pdTRUE) {
        return 0;
    }
    
    return 1; // No event available
}

void midi_cleanup(void) {
    if (!initialized) {
        return;
    }
    
    if (midi_task_handle != NULL) {
        vTaskDelete(midi_task_handle);
        midi_task_handle = NULL;
    }
    
    if (midi_queue != NULL) {
        vQueueDelete(midi_queue);
        midi_queue = NULL;
    }
    
    uart_driver_delete(UART_NUM);
    initialized = false;
}

#endif // ESP_PLATFORM
