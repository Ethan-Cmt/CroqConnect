#ifndef CAM_H
#define CAM_H

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_camera.h>

// Fonction d'initialisation de la caméra
void init_camera();

// Fonction de capture et sauvegarde d'une photo
void image_to_mqtt();

void controlImgCaptureTask(bool enable);

void send_mqtt_frame_callback(void *arg);

void send_mqtt_frame();

#endif  // CAM_H
