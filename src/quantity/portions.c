#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"

#include "com/mqtt/client.h"

#include "portions.h"

static const char *TAG = "distrib portions";

#define PORTIONS_NAMESPACE "portions"

esp_err_t write_distribution_portions(const TimerPortions *portions) {
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(PORTIONS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error while opening NVS memort: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_set_blob(nvs_handle, "portions", portions, sizeof(TimerPortions));
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error while validating NVS changes: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGE(TAG, "Error while writing data into NVS memory: %s", esp_err_to_name(ret));
    }

    nvs_close(nvs_handle);
    return ret;
}

esp_err_t read_distribution_portions(TimerPortions *portions) {
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(PORTIONS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    size_t required_size;
    ret = nvs_get_blob(nvs_handle, "portions", NULL, &required_size);
    if (ret == ESP_OK && required_size == sizeof(TimerPortions)) {
        ret = nvs_get_blob(nvs_handle, "portions", portions, &required_size);
    } else {
        ret = ESP_ERR_NVS_NOT_FOUND;
    }

    nvs_close(nvs_handle);
    return ret;
}

TimerPortions get_distrib_quantity() {
    TimerPortions mem_portions;

    esp_err_t ret = read_distribution_portions(&mem_portions);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Portions successfully read in NVS memory");
    } else {
        ESP_LOGE(TAG, "Error while reading portions from NVS memory");
    }
    return mem_portions;
}

void update_portions_from_json(const char *json_string) {
    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        ESP_LOGE(TAG, "Error while parsing JSON");
        return;
    }

    TimerPortions new_portions;

    // Every portion init w/ empty value
    new_portions.portion_1 = new_portions.portion_2 = new_portions.portion_3 = new_portions.portion_4 = new_portions.portion_5 = new_portions.portion_6 = 0;

        for (int i = 1; i <= 6; i++) {
        char key[10];
        sprintf(key, "portion%d", i);

        cJSON *portion = cJSON_GetObjectItemCaseSensitive(root, key);
        if (portion != NULL) {
            int new_quantity;
            if (sscanf(portion->valuestring, "%d", &new_quantity) == 1) {
                switch (i) {
                    case 1:
                        new_portions.portion_1 = new_quantity;
                        break;
                    case 2:
                        new_portions.portion_2 = new_quantity;
                        break;
                    case 3:
                        new_portions.portion_3 = new_quantity;
                        break;
                    case 4:
                        new_portions.portion_4 = new_quantity;
                        break;
                    case 5:
                        new_portions.portion_5 = new_quantity;
                        break;
                    case 6:
                        new_portions.portion_6 = new_quantity;
                        break;
                    default:
                        break;
                }
            }
        }
    }

    cJSON_Delete(root);

    esp_err_t ret = write_distribution_portions(&new_portions);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Quantities updated successfully");
    } else {
        ESP_LOGE(TAG, "Error occured while updating quantities : %s", esp_err_to_name(ret));
    }
    TimerPortions current_quantities;
    esp_err_t retu = read_distribution_portions(&current_quantities);
    if (retu == ESP_OK) {
        ESP_LOGI(TAG, "Current saved portion(s) :");
        ESP_LOGI(TAG, "Portion 1 : %02d", current_quantities.portion_1);
        ESP_LOGI(TAG, "Portion 2 : %02d", current_quantities.portion_2);
        ESP_LOGI(TAG, "Portion 3 : %02d", current_quantities.portion_3);
        ESP_LOGI(TAG, "Portion 4 : %02d", current_quantities.portion_4);
        ESP_LOGI(TAG, "Portion 5 : %02d", current_quantities.portion_5);
        ESP_LOGI(TAG, "Portion 6 : %02d", current_quantities.portion_6);
    } else {
        ESP_LOGE(TAG, "Error while reading quantities from NVS memory : %s", esp_err_to_name(ret));
    }
    send_portions_to_mqtt();
}

char *get_portions_string(int portion) {

    char *portion_string = (char *)malloc(6);
    if (portion_string == NULL) {
        ESP_LOGE(TAG, "Allocation memory error");
        return NULL;
    }

    snprintf(portion_string, 6, "%02d", portion);

    return portion_string;
}

char *get_portions_json() {
    TimerPortions current_quan;
    esp_err_t ret = read_distribution_portions(&current_quan);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error while reading portions from NVS memory");
        return NULL;
    }

    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "quantity1", get_portions_string(current_quan.portion_1));
    cJSON_AddStringToObject(root, "quantity2", get_portions_string(current_quan.portion_2));
    cJSON_AddStringToObject(root, "quantity3", get_portions_string(current_quan.portion_3));
    cJSON_AddStringToObject(root, "quantity4", get_portions_string(current_quan.portion_4));
    cJSON_AddStringToObject(root, "quantity5", get_portions_string(current_quan.portion_5));
    cJSON_AddStringToObject(root, "quantity6", get_portions_string(current_quan.portion_6));

    char *json_string = cJSON_PrintUnformatted(root);

    cJSON_Delete(root);

    return json_string;
}

void send_portions_callback(void *arg) {
    send_portions_to_mqtt();
}

void periodic_portions_send() {
    const esp_timer_create_args_t portions_sender_args = {
        .callback = &send_portions_callback,
        .arg = NULL,
        .name = "check_dis_quan"
    };
    esp_timer_handle_t periodic_portions_sender;
    esp_timer_create(&portions_sender_args, &periodic_portions_sender);
    esp_timer_start_periodic(periodic_portions_sender, 60 * 1000 * 1000);
}