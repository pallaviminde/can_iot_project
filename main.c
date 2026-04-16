#include <stdio.h>               // Standard I/O functions
#include "freertos/FreeRTOS.h"   // FreeRTOS definitions
#include "freertos/task.h"       // FreeRTOS task functions
#include "esp_log.h"             // ESP logging library
#include "can_driver.h"          // CAN driver header
#include "motor_control.h"       // Motor control functions
#include "wifi_manager.h"        // WiFi manager functions
#include "mqtt_manager.h"        // MQTT manager functions

// Tag used for logging messages from this file
static const char *TAG = "MAIN";

void app_main(void)
{
    /* Initialize CAN */
    // Initialize CAN (TWAI) driver for communication with motor controller
    if (can_init() != ESP_OK) {
        // Log error and stop execution if initialization fails
        ESP_LOGE(TAG, "CAN init failed! Halting.");
        return;
    }
    ESP_LOGI(TAG, "CAN bus ready");

    /* Connect to WiFi */
    // Initialize and connect to configured WiFi network
    if (wifi_manager_init() != ESP_OK) {
        // Stop execution if WiFi connection fails
        ESP_LOGE(TAG, "WiFi init failed! Halting.");
        return;
    }
    ESP_LOGI(TAG, "WiFi ready");

    /* Connect to HiveMQ and start listening for MQTT commands */
    // Connect to MQTT broker (HiveMQ) and start communication 
    if (mqtt_manager_init() != ESP_OK) {
        // Stop execution if MQTT setup fails
        ESP_LOGE(TAG, "MQTT init failed! Halting.");
        return;
    }
    ESP_LOGI(TAG, "MQTT ready – waiting for commands from HiveMQ...");

    /*
     * MQTT Topics for Motor Control:
     * Topic: "motor/enable"
     *   0 -> Disable motor
     *   1 -> Enable motor
     * Topic: "motor/speed"
     *   Range: 0.0 to 50.0 (float value)
     */

    while (1) {
        // Keep main task alive; actual work handled by MQTT task
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay for 1 second
    }
}