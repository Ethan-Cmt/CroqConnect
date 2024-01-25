#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"  // Ajout de l'inclusion pour SemaphoreHandle_t
#include "esp_log.h"

extern SemaphoreHandle_t mqttConnectedSemaphore;
extern SemaphoreHandle_t imageUploadSemaphore;

void app_main();