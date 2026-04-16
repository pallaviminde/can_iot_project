#include "wifi_manager.h"          // WiFi management functions
#include "esp_wifi.h"              // WiFi driver
#include "esp_event.h"             // Event handling
#include "esp_log.h"               // ESP logging library
#include "nvs_flash.h"             // Non-volatile storage (for WiFi credentials)
#include "freertos/FreeRTOS.h"     // FreeRTOS definitions
#include "freertos/event_groups.h" // Event group for WiFi connection status
#include "freertos/timers.h"       // FreeRTOS timers for reconnection logic

// Logging tag
static const char *TAG = "WIFI";

// Bit to indicate WiFi connection established
#define WIFI_CONNECTED_BIT      BIT0
// Initial reconnect delay (2 seconds)
#define WIFI_RECONNECT_DELAY_MS 2000 
// Maximum reconnect delay (60 seconds)  
#define WIFI_RECONNECT_MAX_MS   60000  

// Event group handle for WiFi status
static EventGroupHandle_t s_wifi_event_group;
// Timer used for delayed reconnection
static TimerHandle_t      s_reconnect_timer = NULL;
// Current reconnect delay (used for exponential backoff)
static uint32_t           s_reconnect_delay = WIFI_RECONNECT_DELAY_MS;
// Connection status flag
static bool               s_connected       = false;
// Forward declaration
static void start_reconnect_timer(void);
/*
 * Function: reconnect_timer_cb
 * Callback function triggered when reconnect timer expires.
 * Attempts to reconnect to WiFi if not already connected.
 */
static void reconnect_timer_cb(TimerHandle_t xTimer)
{
    if (!s_connected) {
        ESP_LOGI(TAG, "Auto-reconnecting to WiFi...");
        esp_wifi_connect();
    }
}
/*
 * Function: start_reconnect_timer
 * Starts or updates a reconnect timer using exponential backoff.
 * Delay doubles after each failure up to a maximum limit.
 */
static void start_reconnect_timer(void)
{
    ESP_LOGW(TAG, "Reconnect in %lu ms...", (unsigned long)s_reconnect_delay);

    if (s_reconnect_timer == NULL) {
        // Create one-shot timer for reconnect
        s_reconnect_timer = xTimerCreate("wifi_reconnect",
                                          pdMS_TO_TICKS(s_reconnect_delay),
                                          pdFALSE,  
                                          NULL,
                                          reconnect_timer_cb);
    } else {
        // Update timer period if already created
        xTimerChangePeriod(s_reconnect_timer,
                           pdMS_TO_TICKS(s_reconnect_delay), 0);
    }
    // Start the timer
    xTimerStart(s_reconnect_timer, 0);

    // Exponential backoff: double delay 
    s_reconnect_delay *= 2;
    // Limit maximum delay
    if (s_reconnect_delay > WIFI_RECONNECT_MAX_MS) {
        s_reconnect_delay = WIFI_RECONNECT_MAX_MS;
    }
}

/*
 * Function: wifi_event_handler
 * Handles WiFi and IP events:
 * - Connect on start
 * - Handle disconnection and trigger reconnect
 * - Handle successful IP acquisition
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {

        if (event_id == WIFI_EVENT_STA_START) {
            // Station mode started → connect to AP
            ESP_LOGI(TAG, "STA started – connecting...");
            esp_wifi_connect();

        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            // WiFi disconnected
            s_connected = false;
            wifi_event_sta_disconnected_t *disc =
                (wifi_event_sta_disconnected_t *)event_data;
            ESP_LOGW(TAG, "Disconnected (reason: %d)", disc->reason);

            // Start reconnect mechanism
            start_reconnect_timer();
        }

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // Successfully got IP address
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));

        s_connected = true;

        // Reset reconnect delay after success
        s_reconnect_delay = WIFI_RECONNECT_DELAY_MS;

        // Stop reconnect timer if running
        if (s_reconnect_timer != NULL) {
            xTimerStop(s_reconnect_timer, 0);
        }
        // Notify tasks that WiFi is connected
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/*
 * Function: wifi_manager_init
 * Initializes WiFi subsystem and connects to access point.
 * Steps:
 * 1. Initialize NVS (required for WiFi)
 * 2. Initialize network interface and event loop
 * 3. Configure WiFi in station mode
 * 4. Register event handlers
 * 5. Start WiFi
 * 6. Block until connected
 * returns: ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_init(void)
{
    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();

    // Handle NVS errors (erase and retry)
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) return ret;

    // Create event group for WiFi status tracking
    s_wifi_event_group = xEventGroupCreate();

    // Initialize TCP/IP stack and event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default WiFi station interface
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register WiFi and IP event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    // Configure WiFi credentials
    wifi_config_t wifi_config = {
        .sta = {
            .ssid               = WIFI_SSID,
            .password           = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    // Set WiFi mode and configuration, then start WiFi
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Apply WiFi configuration and start WiFi
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Start WiFi and connect to AP
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Waiting for first WiFi connection...");

    // Block until connected to WiFi
    xEventGroupWaitBits(s_wifi_event_group,
                        WIFI_CONNECTED_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);

    return ESP_OK;
}