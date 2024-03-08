#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include "driver/gpio.h"
#include "fountain.h"

// Définir la broche GPIO pour contrôler le relais
#define PIN_RELAY GPIO_NUM_21

static const char *TAG = "fountain";

void init_fountain() {
    gpio_set_direction(PIN_RELAY, GPIO_MODE_OUTPUT);
}

void turn_fountain_on() {
    ESP_LOGI(TAG, "Fountain ON");
    gpio_set_level(PIN_RELAY, 1);
}

void turn_fountain_off() {
    ESP_LOGI(TAG, "Fountain OFF");
    gpio_set_level(PIN_RELAY, 0);
}