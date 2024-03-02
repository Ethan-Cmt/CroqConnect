#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "time/time.h"
#include "com/mqtt/client.h"
#include "distrib/motor.h"
#include "camera/cam.h"
#include "com/mqtt/client.h"

static const char *TAG = "distrib";

void distribute_croquettes(int quantity) {
    ESP_LOGI(TAG, "Distribution...");
    char* current_time_str = get_current_time_string();
    int rotations = quantity/10;
    ESP_LOGI(TAG, "rotations : %d", rotations);

    for (int i = 0; i < rotations; ++i) {
        motor_set_angle(179);
        motor_set_angle(-179); // May change depending on the angle for 10g. Maybe encapsuled in task w/ big priority
    }

    if (current_time_str != NULL) {
        char message[100];
        snprintf(message, sizeof(message), "last ditrib : %s", current_time_str);

        mqtt_publish_message("distribution/last", message, 1);

        free(current_time_str);
    } else {
        ESP_LOGE(TAG, "Erreur while trying to get current time");
    }
    send_quantity_to_mqtt();
    ESP_LOGI(TAG, "Distribution done.");
}


