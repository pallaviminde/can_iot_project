#ifndef MOTOR_CONTROL_H  // Include guard to prevent multiple inclusion
#define MOTOR_CONTROL_H
 
/*
 * Function: motor_enable
 * Sends command to enable/start the motor.
 */
void motor_enable(void);
/*
 * Function: motor_disable
 * Sends command to disable/stop the motor.
 */
void motor_disable(void);
/*
 * Function: motor_set_speed
 * Sets motor speed.
 *
 * speed: Desired motor speed in rotations per second (r/s)
 *        Valid range is defined in implementation file.
 */
void motor_set_speed(float speed);
 
#endif