#ifndef MQTT_MANAGER_H  // Prevents multiple inclusion of this header file
#define MQTT_MANAGER_H
#include "esp_err.h"    // ESP error codes

#define MQTT_BROKER_URI      "mqtt://broker.hivemq.com"
/*#define MQTT_USERNAME        "YOUR_HIVEMQ_USERNAME"
#define MQTT_PASSWORD        "YOUR_HIVEMQ_PASSWORD"
#define MQTT_CLIENT_ID       "esp32_motor_node"*/

/* ── Topics subscribed by the ESP32  */
// Topic to control motor enable/disable
// Expected JSON: { "enable": 0 or 1 }
#define TOPIC_MOTOR_ENABLE   "motor/enable" 

// Topic to control motor speed
// Expected JSON: { "speed": float value }
#define TOPIC_MOTOR_SPEED    "motor/speed"    

/* ── Topic the ESP32 publishes status on */
// Topic used to send status updates back to broker
#define TOPIC_MOTOR_STATUS   "motor/status"

/*
 * Function: mqtt_manager_init
 * Initializes MQTT client, connects to broker,
 * and starts communication.
 * returns: ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t mqtt_manager_init(void);

#endif 