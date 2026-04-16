#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"  // ESP error codes

// WiFi network SSID (name of the access point)
#define WIFI_SSID       "V2036"
// WiFi network password (must match the access point's password)
#define WIFI_PASSWORD   "pallavi@12324"
// Maximum retry attempts
#define WIFI_MAX_RETRY  5

esp_err_t wifi_manager_init(void);

#endif 