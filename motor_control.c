#include "motor_control.h"
#include "can_driver.h"    // CAN transmit function
#include "esp_log.h"       // ESP logging library
#include <string.h>        // For memcpy function

// Tag used for logging messages from this file
static const char *TAG = "MOTOR";

/* ----------- CAN Command Identifiers ----------- */
// Command to disable motor
#define CMD_DISABLE  0x040
// Command to enable motor
#define CMD_ENABLE   0x041
// Command to set motor speed
#define CMD_SPEED    0x043

/* ----------- Motor Speed Limits ----------- */
// Minimum allowed motor speed (r/s)
#define MOTOR_SPEED_MIN  0.0f
// Maximum allowed motor speed (r/s)
#define MOTOR_SPEED_MAX  50.0f

/*
 * Function: motor_enable
 * Sends CAN command to enable the motor.
 */
void motor_enable(void)
{
    // Log action
    ESP_LOGI(TAG, "Motor Enable");
    // Send CAN command with no data payload
    can_send(CMD_ENABLE, NULL, 0);
}
 
/*
 * Function: motor_disable
 * Sends CAN command to disable the motor.
 */
void motor_disable(void)
{
    // Log action
    ESP_LOGI(TAG, "Motor Disable");
    // Send CAN command with no data payload
    can_send(CMD_DISABLE, NULL, 0);
}

/*
 * Function: motor_set_speed
 * Sets motor speed via CAN communication.
 * speed: Desired motor speed in rotations per second (r/s)
 * Notes:
 * - Speed is clamped within allowed range (0.0 to 50.0 r/s)
 * - Speed is transmitted as 4-byte float over CAN
 */
 
void motor_set_speed(float speed)
{
    // Check and clamp speed if below minimum limit
    if (speed < MOTOR_SPEED_MIN) {
        ESP_LOGW(TAG, "Speed %.2f too low, clamping to %.2f", speed, MOTOR_SPEED_MIN);
        speed = MOTOR_SPEED_MIN;
    }
    // Check and clamp speed if above maximum limit
    if (speed > MOTOR_SPEED_MAX) {
        ESP_LOGW(TAG, "Speed %.2f too high, clamping to %.2f", speed, MOTOR_SPEED_MAX);
        speed = MOTOR_SPEED_MAX;
    }
 
    // Prepare data buffer to hold the float speed value as bytes
    uint8_t data[4];
    // Copy the float speed value into the data buffer (4 bytes)
    memcpy(data, &speed, 4);
    // Log the final speed being sent
    ESP_LOGI(TAG, "Set Speed: %.2f r/s", speed);
    // Send speed command with 4-byte payload
    can_send(CMD_SPEED, data, 4);
}