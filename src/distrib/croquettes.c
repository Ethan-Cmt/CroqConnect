#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "time/time.h"
#include "com/mqtt/client.h"
#include "distrib/motor.h"
#include "camera/cam.h"

static const char *TAG = "distrib";

void distribute_croquettes() { // TODO : Add paramaeter 'portion' which corresponds to a precise quantity
    ESP_LOGI(TAG, "Distribution...");
    char* current_time_str = get_current_time_string();
    motor_set_angle(89); // May change

    if (current_time_str != NULL) {
        char message[100];
        snprintf(message, sizeof(message), "last ditrib : %s", current_time_str);

        mqtt_publish_message("distribution/last", message, 1);

        free(current_time_str);
    } else {
        ESP_LOGE(TAG, "Erreur while trying to get current time");
    }
    ESP_LOGI(TAG, "Distribution done."); // TODO : Add HX711 diff check
}
