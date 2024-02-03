#ifndef CAM_H
#define CAM_H

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_camera.h>

// Fonction d'initialisation de la caméra
void init_camera();

// Fonction de capture et sauvegarde d'une photo
void image_to_mqtt(void *pvParameters);

void controlImgCaptureTask(bool enable);

#endif  // CAM_H
