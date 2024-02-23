#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "time/time.h"
#include "com/mqtt/client.h"
#include "distrib/motor.h"
#include "camera/cam.h"

static const char *TAG = "distrib";

void distribute_croquettes() {
    ESP_LOGI(TAG, "Distribution en cours...");
    char* current_time_str = get_current_time_string();
    motor_set_angle(89);
    //image_to_mqtt();

    if (current_time_str != NULL) {
        char message[100];
        snprintf(message, sizeof(message), "last ditrib : %s", current_time_str);

        mqtt_publish_message("distribution/last", message, 1);

        // Libérer la mémoire allouée par get_current_time_string
        free(current_time_str);
    } else {
        ESP_LOGE(TAG, "Erreur lors de l'obtention de l'heure actuelle.");
    }
    ESP_LOGI(TAG, "Distribution terminée."); //Ajouter verif HX711
}
