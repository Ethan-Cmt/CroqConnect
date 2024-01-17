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
#include "distrib/motor.h"
#include "camera/cam.h"

void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    initialize_camera();
}