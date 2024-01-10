#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"

#define SERVO_PIN GPIO_NUM_12

void initialize_servo() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL<<SERVO_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);
}

void control_servo(int angle) {
    gpio_set_level(SERVO_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(1000 + angle * 10));
    gpio_set_level(SERVO_PIN, 0);
}

void distribute_croquettes() {
    control_servo(90);
    vTaskDelay(pdMS_TO_TICKS(250));
    control_servo(0);
}