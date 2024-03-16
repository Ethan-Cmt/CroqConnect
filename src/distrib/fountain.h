#ifndef FOUNTAIN_H
#define FOUNTAIN_H

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#define RELAY_PIN GPIO_NUM_2

void init_fountain();
void turn_fountain_on();
void turn_fountain_off();
void toggle_fountain();

#endif // FOUNTAIN_H
