#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "com/wifi/wifi.h"
#include "com/mqtt/client.h"
#include "time/time.h"
#include "camera/cam.h"
#include "distrib/motor.h"

void app_main()
{
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

            xTaskCreatePinnedToCore(mqtt_task, "mqtt check", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY + 1, NULL, 1);
            xTaskCreatePinnedToCore(image_to_mqtt, "send img to broker", 4096, NULL, 5, NULL, 1);
            break;
        }
    }
}