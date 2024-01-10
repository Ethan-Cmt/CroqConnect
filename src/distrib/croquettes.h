#ifndef CROQUETTES_H
#define CROQUETTES_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"

#define SERVO_PIN GPIO_NUM_12

void initialize_servo();
void control_servo(int angle);
void distribute_croquettes();

#endif  // CROQUETTES_H
