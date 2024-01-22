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
void capture_and_save_photo();

#endif  // CAM_H
