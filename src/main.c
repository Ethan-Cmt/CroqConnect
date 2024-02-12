#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "main.h"
#include "com/wifi/wifi.h"
#include "com/mqtt/client.h"
#include "time/time.h"
#include "camera/cam.h"
#include "distrib/motor.h"
#include "quantity/hx711.h"

#define TAG "MAIN"

SemaphoreHandle_t mqttConnectedSemaphore;
SemaphoreHandle_t imageUploadSemaphore;

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    mqttConnectedSemaphore = xSemaphoreCreateBinary();
    if (mqttConnectedSemaphore == NULL) {
        ESP_LOGE(TAG, "Impossible de créer le sémaphore de connexion MQTT");
        vTaskDelay(portMAX_DELAY);
    }

    imageUploadSemaphore = xSemaphoreCreateBinary();
    if (imageUploadSemaphore == NULL) {
        ESP_LOGE(TAG, "Impossible de créer le sémaphore pour l'envoi d'image");
        vTaskDelay(portMAX_DELAY);
    }

    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_sta();

    // Attendre jusqu'à ce que l'adresse IP soit obtenue (timeout de 10 secondes)
    for (int i = 0; i < 100; i++)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        if (ip_obtained)
        {
            initialize_sntp();
            wait_for_time();
            initialize_time();
            motor_init();
            init_camera();
            mqtt_app_start();

            //xTaskCreate(test, "test", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
            //xTaskCreatePinnedToCore(mqtt_task, "mqtt check", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY + 1, NULL, 1);
            //xTaskCreatePinnedToCore(image_to_mqtt, "send img to broker", 4096, NULL, 5, NULL, 1);
            //periodic_time_check();
            break;
        }
    }
}