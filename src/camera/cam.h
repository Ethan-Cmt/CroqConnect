#ifndef CAM_H
#define CAM_H

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_camera.h>

// Définissez le tag pour les logs
#define TAG "CAMERA_APP"

// Fonction d'initialisation de la caméra
void init_camera();

// Fonction de capture et sauvegarde d'une photo
void image_to_mqtt(void *pvParameters);

void init_sd();

#endif  // CAM_H
