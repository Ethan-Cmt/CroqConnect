#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_camera.h"

#include "com/mqtt/client.h"
#include "cam.h"
#include "main.h"

#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS portTICK_PERIOD_MS
#endif
#include <esp_timer.h>

#define TAG "CAMERA_APP"

#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 15
#define SIOD_GPIO_NUM 4
#define SIOC_GPIO_NUM 5

#define Y2_GPIO_NUM 11
#define Y3_GPIO_NUM 9
#define Y4_GPIO_NUM 8
#define Y5_GPIO_NUM 10
#define Y6_GPIO_NUM 12
#define Y7_GPIO_NUM 18
#define Y8_GPIO_NUM 17
#define Y9_GPIO_NUM 16

#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM 7
#define PCLK_GPIO_NUM 13

static TaskHandle_t imgCaptureTaskHandle = NULL;

static bool imgCaptureTaskEnabled = false;

static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sccb_sda = SIOD_GPIO_NUM,
    .pin_sccb_scl = SIOC_GPIO_NUM,

    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,

    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, 
    .frame_size = FRAMESIZE_QVGA,    // Do not use sizes above QVGA when not JPEG.

    .fb_location = CAMERA_FB_IN_DRAM,
    .jpeg_quality = 15, // 0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1,      // When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

void init_camera() {
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera initialization failed with error 0x%x", err);
        vTaskDelay(portMAX_DELAY);
    }
}

void image_to_mqtt() {
    if (!mqtt_connected) { 
        ESP_LOGE(TAG, "MQTT connection not established. Cannot send image.");
        vTaskDelay(100 / portTICK_PERIOD_MS);
        return;
    }

    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) {
        if (fb->buf) {
            ESP_LOGI(TAG, "Sending image with MQTT...");
            send_image_data(fb->buf, fb->len);
        } else {
            ESP_LOGE(TAG, "Framebuffer data is NULL");
        }
        esp_camera_fb_return(fb);
    } else {
        ESP_LOGE(TAG, "Camera capture failed");
    }
}

void send_mqtt_frame_callback(void *arg) {
    while (1) {
        image_to_mqtt();
        //vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

// Periodic image sending w/ MQTT
void send_mqtt_frame() { 
    const esp_timer_create_args_t mqtt_frame_args = {
        .callback = &send_mqtt_frame_callback,
        .arg = NULL,
        .name = "image_to_mqtt"
    };
    esp_timer_handle_t periodic_frame_send;
    esp_timer_create(&mqtt_frame_args, &periodic_frame_send);
    esp_timer_start_periodic(periodic_frame_send, 0.25 * 1000 * 1000);
    vTaskDelete(NULL);
}

// Not Used
void controlImgCaptureTask(bool enable) { 
    if (enable) {
        if (!imgCaptureTaskEnabled) {
            xTaskCreatePinnedToCore(image_to_mqtt, "send img to broker", 4096, NULL, 5, &imgCaptureTaskHandle, 1);
            imgCaptureTaskEnabled = true;
        }
    } else {
        if (imgCaptureTaskEnabled) {
            if (imgCaptureTaskHandle != NULL) {
                vTaskDelete(imgCaptureTaskHandle);
                imgCaptureTaskHandle = NULL;
            }
            imgCaptureTaskEnabled = false;
        }
    }
}