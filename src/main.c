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
#include "distrib/fountain.h"
#include "quantity/hx711.h"
#include "quantity/portions.h"


#define TAG "MAIN"

SemaphoreHandle_t mqttConnectedSemaphore; 
SemaphoreHandle_t imageUploadSemaphore;

void app_main()
{
    // NVS Flash memory init
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_log_level_set("*", ESP_LOG_DEBUG); //Log level set to DEBUG

    mqttConnectedSemaphore = xSemaphoreCreateBinary();
    if (mqttConnectedSemaphore == NULL) {
        ESP_LOGE(TAG, "Couldn't create MQTT connect semaphore");
        vTaskDelay(portMAX_DELAY);
    }

    imageUploadSemaphore = xSemaphoreCreateBinary();
    if (imageUploadSemaphore == NULL) {
        ESP_LOGE(TAG, "Couldn't create img upload semaphore");
        vTaskDelay(portMAX_DELAY);
    }
    else {
        xSemaphoreGive(imageUploadSemaphore);
    }


    motor_init();
    init_camera();
    init_fountain();
    init_tare();

    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_sta(); // Try to connect to wifi w/ saved credentials

    // Waiting to get IP adress until 60 secs
    for (int i = 0; i < 900; i++)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        if (ip_obtained)
        {
            initialize_sntp();
            wait_for_time();
            initialize_time();
            mqtt_app_start();

            periodic_time_check(); // May be replaced by C Timer(s) OR Task on core 1 w/ high priority
            while (1) { // MQTT sender loop
                if (mqtt_connected) {
                    xTaskCreatePinnedToCore(send_mqtt_frame_callback, "img_sndr", 4096, NULL, 5, NULL, 1); // Test this for others periodic tasks
                    periodic_schedule_send();
                    periodic_portions_send();
                    periodic_quantity_send();
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            while (1) {
                vTaskDelay(pdMS_TO_TICKS(250));
            }
        }
    }
}