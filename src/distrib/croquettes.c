#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "time/time.h"
#include "com/mqtt/client.h"
#include "distrib/motor.h"

static const char *TAG = "distrib";

void distribute_croquettes() {
    char* current_time_str = get_current_time_string();

    if (current_time_str != NULL) {
        char message[100];
        snprintf(message, sizeof(message), "last ditrib : %s", current_time_str);

        mqtt_publish_message("distribution/last", message, 1);

        // Libérer la mémoire allouée par get_current_time_string
        free(current_time_str);
        motor_set_angle(89);
        motor_set_angle(-89);

    } else {
        ESP_LOGE(TAG, "Erreur lors de l'obtention de l'heure actuelle.");
    }
}
