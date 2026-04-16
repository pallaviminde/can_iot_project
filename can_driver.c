#include "can_driver.h"
#include "driver/twai.h"  // TWAI (CAN) driver
#include "esp_log.h"      // Logging library
#include <string.h>       

// Define GPIO pins used for CAN TX and RX
#define TX_GPIO 16
#define RX_GPIO 15
 
// Tag used for ESP logging
static const char *TAG = "CAN";

esp_err_t can_init(void)
{
    // Configure general TWAI settings (TX, RX pins and mode)
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO, RX_GPIO, TWAI_MODE_NORMAL);
    // Configure timing for 500 Kbps communication speed
    twai_timing_config_t  t_config = TWAI_TIMING_CONFIG_500KBITS();
    // Configure filter to accept all messages
    twai_filter_config_t  f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
 
    esp_err_t ret;
    // Install TWAI driver with above configurations
    ret = twai_driver_install(&g_config, &t_config, &f_config);
    if (ret != ESP_OK) {
        // Log error if driver installation fails
        ESP_LOGE(TAG, "Driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Start TWAI driver
    ret = twai_start();
    if (ret != ESP_OK) {
        // Log error if driver start fails
        ESP_LOGE(TAG, "TWAI start failed: %s", esp_err_to_name(ret));
        return ret;
    }
    // Log successful initialization
 
    ESP_LOGI(TAG, "CAN Initialized");
    return ESP_OK;
}

/*
 * Function: can_send
 * Sends a CAN message using TWAI driver.
 * id   : CAN message identifier
 * data : Pointer to data buffer
 * len  : Number of bytes to send (max 8 for standard CAN)
 */
void can_send(uint32_t id, uint8_t *data, uint8_t len)
{
    // Validate input: data pointer must not be NULL if length > 0
    if (data == NULL && len > 0) {
        ESP_LOGE(TAG, "can_send: data NULL len=%d ", len);
        return;
    }
    // Create TWAI message structure
    twai_message_t msg;
    // Clear message structure to avoid garbage values
    memset(&msg, 0, sizeof(msg));
 
    // Set CAN message ID and data length code
    msg.identifier       = id;
    msg.data_length_code = len;
 
    // Copy data into CAN message buffer
    for (int i = 0; i < len; i++) {
        msg.data[i] = data[i];
    }
    // Transmit message with timeout of 1000 ms
 
    if (twai_transmit(&msg, pdMS_TO_TICKS(1000)) == ESP_OK) {
        // Log success message
        ESP_LOGI(TAG, "Message Sent (ID: 0x%lx)", id);
    } else {
        // Log failure if transmission fails
        ESP_LOGE(TAG, "Send Failed");
    }
}