#ifndef CAN_DRIVER_H  // Include guard to prevent multiple inclusion
#define CAN_DRIVER_H  
 
#include <stdint.h>    // Standard integer types
#include "esp_err.h"  // ESP error codes
/*
 * Function: can_init
 * Initializes the CAN (TWAI) driver with predefined configuration.
 * returns: ESP_OK on success, or an error code on failure
 */

esp_err_t can_init(void);
 
/*
 * Function: can_send
 * Sends a CAN message.
 * id   : CAN message identifier (standard or extended)
 * data : Pointer to data buffer to be transmitted
 * len  : Length of data in bytes (maximum 8 bytes for standard CAN)
 */
void can_send(uint32_t id, uint8_t *data, uint8_t len);
 
#endif // CAN_DRIVER_H