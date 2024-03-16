#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include "driver/gpio.h"
#include "fountain.h"

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

void toggle_fountain() {
    gpio_set_direction(PIN_RELAY, GPIO_MODE_INPUT);
    int current_state = gpio_get_level(PIN_RELAY);
    ESP_LOGI(TAG, "Relay PIN state : %d", current_state);
    if (current_state == 0) {
        turn_fountain_on();
    } else {
        turn_fountain_off();
    }
    gpio_set_direction(PIN_RELAY, GPIO_MODE_OUTPUT);
}