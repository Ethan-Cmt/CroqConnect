#ifndef CAM_H
#define CAM_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/gpio.h"
#include "sdmmc_cmd.h"
#include "diskio.h"
#include "ff.h"
#include "esp_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

// Déclaration de la fonction d'initialisation de la caméra
void initialize_camera();

// Déclaration de la fonction de capture et de sauvegarde des images
void capture_and_save_images();

#ifdef __cplusplus
}
#endif

#endif /* CAM_H */
