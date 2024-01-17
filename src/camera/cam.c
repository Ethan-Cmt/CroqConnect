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

static const char *TAG = "camera";

void initialize_camera() {
    camera_config_t camera_config = {
        .pin_pwdn = -1,
        .pin_reset = -1,
        .pin_xclk = 40,
        .pin_sscb_sda = 26,
        .pin_sscb_scl = 27,

        .pin_d7 = 35,
        .pin_d6 = 34,
        .pin_d5 = 39,
        .pin_d4 = 36,
        .pin_d3 = 21,
        .pin_d2 = 19,
        .pin_d1 = 18,
        .pin_d0 = 5,
        .pin_vsync = 21,
        .pin_href = 38,
        .pin_pclk = 11,
    };

    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Initialisation de la caméra échouée : %d", err);
        return;
    }

    ESP_LOGI(TAG, "Caméra initialisée avec succès");
}

void capture_and_save_images() {
    // ... Ajoutez votre logique de capture et de sauvegarde ici

    while (1) {
        // Capturez une image
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Impossible de capturer l'image");
            continue;
        }

        // Enregistrez l'image sur la carte SD
        char image_path[32];
        snprintf(image_path, sizeof(image_path), "/sdcard/image_%llu.jpg", (uint64_t)esp_timer_get_time());

        FILE *file = fopen(image_path, "w");
        if (!file) {
            ESP_LOGE(TAG, "Impossible d'ouvrir le fichier pour écrire");
            esp_camera_fb_return(fb);
            continue;
        }

        size_t size = fwrite(fb->buf, 1, fb->len, file);
        fclose(file);

        esp_camera_fb_return(fb);

        if (size != fb->len) {
            ESP_LOGE(TAG, "Erreur lors de l'écriture de l'image sur la carte SD");
            continue;
        }

        ESP_LOGI(TAG, "Image capturée et sauvegardée : %s", image_path);

        // Ajoutez ici tout autre traitement ou temporisation si nécessaire
    }
}