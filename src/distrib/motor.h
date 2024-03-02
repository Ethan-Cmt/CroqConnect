#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include <stdint.h>

extern int current_angle;

void motor_init();
void motor_set_angle(int angle);

#endif // SERVO_CONTROL_H
