#ifndef FOUNTAIN_H
#define FOUNTAIN_H

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

// Définir les broches GPIO pour le contrôle du relais
#define RELAY_PIN GPIO_NUM_2  // Remplacez cela par le numéro de broche que vous utilisez

// Prototypes de fonctions
void init_fountain();
void turn_fountain_on();
void turn_fountain_off();

#endif // FOUNTAIN_H
