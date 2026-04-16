#include "mqtt_manager.h"
#include "motor_control.h"  // Motor control functions
#include "esp_log.h"        // ESP logging library
#include "mqtt_client.h"    // MQTT client functions
#include <string.h>        // For string manipulation functions
#include <stdlib.h>        // For dynamic memory management
#include <stdio.h>         // For snprintf function
#include "cJSON.h"        // cJSON library for JSON parsing and creation

// Logging tag
static const char *TAG = "MQTT";
// MQTT client handle (global for publish access)
static esp_mqtt_client_handle_t s_client = NULL;

/*
 * Function: publish_status
 * Publishes a JSON status message to MQTT broker.
 * msg: Status message string
 * Example JSON:
 * { "enable": 0 }
 */
static void publish_status(const char *msg)
{
    if (s_client) {
        // Create JSON object with status message
        cJSON *root = cJSON_CreateObject();

        // Add "status" field
        cJSON_AddStringToObject(root, "status", msg);

        // Convert JSON object to string
        char *json_str = cJSON_PrintUnformatted(root);

        // Publish to MQTT topic
        esp_mqtt_client_publish(s_client, TOPIC_MOTOR_STATUS, json_str, 0, 1, 0);

        ESP_LOGI(TAG, "Status published: %s", json_str);
        // Free allocated memory for JSON object and string

        cJSON_Delete(root);
        free(json_str);
    }
}

/*
 * Function: handle_mqtt_message
 * Processes incoming MQTT messages:
 * - Parses topic and payload
 * - Decodes JSON
 * - Executes motor control commands
 * topic     : MQTT topic string
 * topic_len : Length of topic
 * data      : Payload data
 * data_len  : Length of payload
 */
static void handle_mqtt_message(const char *topic, int topic_len,
                                 const char *data,  int data_len)
{
    // Buffers to store null-terminated strings
    char topic_str[64]  = {0};
    char data_str[128]  = {0};

    // Convert raw MQTT data to proper C strings
    snprintf(topic_str, sizeof(topic_str), "%.*s", topic_len, topic);
    snprintf(data_str,  sizeof(data_str),  "%.*s", data_len,  data);

    ESP_LOGI(TAG, "RX topic='%s' payload='%s'", topic_str, data_str);

    /* Parse JSON */
    cJSON *root = cJSON_Parse(data_str);
    if (root == NULL) {
        ESP_LOGE(TAG, "Invalid JSON");
        publish_status("ERROR: Invalid JSON");
        return;
    }

    /* motor enable */
    if (strcmp(topic_str, TOPIC_MOTOR_ENABLE) == 0) {

        cJSON *enable = cJSON_GetObjectItem(root, "enable");

        if (cJSON_IsNumber(enable)) {
            if (enable->valueint == 1) {
                motor_enable();
                publish_status("Motor Enabled");
            } else if (enable->valueint == 0) {
                motor_disable();
                publish_status("Motor Disabled");
            } else {
                publish_status("ERROR: enable must be 0 or 1");
            }
        } else {
            publish_status("ERROR: Invalid enable JSON");
        }

        cJSON_Delete(root);
        return;
    }

    /* motor speed */
    if (strcmp(topic_str, TOPIC_MOTOR_SPEED) == 0) {

        cJSON *speed = cJSON_GetObjectItem(root, "speed");

        if (cJSON_IsNumber(speed)) {
            // Set motor speed
            motor_set_speed(speed->valuedouble);

            // Prepare status message with the new speed
            char status_msg[64];
            snprintf(status_msg, sizeof(status_msg),
                     "Speed set to %.2f r/s", speed->valuedouble);

            publish_status(status_msg);
        } else {
            publish_status("ERROR: Invalid speed JSON");
        }

        cJSON_Delete(root);
        return;
    }
    // Unknown topic
    cJSON_Delete(root);
    ESP_LOGW(TAG, "Unhandled topic: '%s'", topic_str);
}

/*
 * Function: mqtt_event_handler
 * Handles MQTT client events such as:
 * - Connection
 * - Disconnection
 * - Incoming data
 * - Errors
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {

    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Connected to HiveMQ broker");

        /* Subscribe to both control topics */
        esp_mqtt_client_subscribe(s_client, TOPIC_MOTOR_ENABLE, 1);
        esp_mqtt_client_subscribe(s_client, TOPIC_MOTOR_SPEED,  1);
        ESP_LOGI(TAG, "Subscribed: %s, %s", TOPIC_MOTOR_ENABLE, TOPIC_MOTOR_SPEED);
        
        // Notify system is online
        publish_status("ESP32 Motor Node Online");
        break;

    case MQTT_EVENT_DISCONNECTED:
        // Automatic reconnect handled by client
        ESP_LOGW(TAG, "Disconnected from broker – will auto-reconnect");
        break;

    case MQTT_EVENT_DATA:
        // Handle incoming MQTT message
        handle_mqtt_message(event->topic, event->topic_len,
                             event->data,  event->data_len);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error type: %d", event->error_handle->error_type);
        break;

    default:
        break;
    }
}

/*
 * Function: mqtt_manager_init
 * Initializes and starts MQTT client.
 * - Configures broker URI
 * - Registers event handler
 * - Starts MQTT service
 * returns: ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t mqtt_manager_init(void)
{
    // MQTT configuration structure
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri      = MQTT_BROKER_URI,
        
        /*.credentials.username    = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASSWORD,
        .credentials.client_id   = MQTT_CLIENT_ID,*/
        // Skip certificate validation (for testing)
        .broker.verification.skip_cert_common_name_check = true,
    };

    // Initialize MQTT client
    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_client == NULL) {
        ESP_LOGE(TAG, "MQTT client init failed");
        return ESP_FAIL;
    }

    // Register event handler
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID,
                    mqtt_event_handler, NULL));
    // Start MQTT client
    ESP_ERROR_CHECK(esp_mqtt_client_start(s_client));

    ESP_LOGI(TAG, "MQTT client started → %s", MQTT_BROKER_URI);
    return ESP_OK;
}