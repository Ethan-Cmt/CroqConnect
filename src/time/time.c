#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"

#include "distrib/croquettes.h"
#include "com/mqtt/client.h"
#include "time.h"

static const char *TAG = "time sync";

#define DISTRIBUTION_SCHEDULE_NAMESPACE "schedule"

// Fonction pour écrire les horaires dans la mémoire flash
esp_err_t write_distribution_schedule(const DistributionSchedule *schedule) {
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(DISTRIBUTION_SCHEDULE_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erreur lors de l'ouverture de la mémoire NVS: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_set_blob(nvs_handle, "schedule", schedule, sizeof(DistributionSchedule));
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Erreur lors de la validation des changements NVS: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGE(TAG, "Erreur lors de l'écriture des données dans la mémoire NVS: %s", esp_err_to_name(ret));
    }

    nvs_close(nvs_handle);
    return ret;
}

// Fonction pour lire les horaires depuis la mémoire flash
esp_err_t read_distribution_schedule(DistributionSchedule *schedule) {
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(DISTRIBUTION_SCHEDULE_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    size_t required_size;
    ret = nvs_get_blob(nvs_handle, "schedule", NULL, &required_size);
    if (ret == ESP_OK && required_size == sizeof(DistributionSchedule)) {
        ret = nvs_get_blob(nvs_handle, "schedule", schedule, &required_size);
    } else {
        ret = ESP_ERR_NVS_NOT_FOUND;
    }

    nvs_close(nvs_handle);
    return ret;
}

void initialize_sntp() {
    ESP_LOGI(TAG, "Initialisation du client SNTP...");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
}

void wait_for_time() {
    ESP_LOGI(TAG, "En attente de la synchronisation de l'heure...");
    time_t now = 0;
    struct tm timeinfo = {0};

    while (timeinfo.tm_year < (2024 - 1900)) {
        time(&now);
        localtime_r(&now, &timeinfo);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Heure synchronisée avec succès");
}

void initialize_time() {
    setenv("TZ", "UTC", 1);
    tzset();

    time_t now;
    struct tm timeinfo;

    time(&now);
    gmtime_r(&now, &timeinfo);

    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "Heure actuelle (UTC) : %s", strftime_buf);

    if (timeinfo.tm_mon >= 3 && timeinfo.tm_mon < 10) {
        setenv("TZ", "CEST-2", 1);
    } else {
        setenv("TZ", "CET-1", 1);
    }

    tzset();

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "Heure actuelle (Fuseau horaire français) : %s", strftime_buf);
}

void check_and_distribute_croquettes(const DistributionSchedule *schedule) {
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_hour == schedule->hour_1 && timeinfo.tm_min == schedule->minute_1) {
        distribute_croquettes();
    } else if (timeinfo.tm_hour == schedule->hour_2 && timeinfo.tm_min == schedule->minute_2) {
        distribute_croquettes();
    } else if (timeinfo.tm_hour == schedule->hour_3 && timeinfo.tm_min == schedule->minute_3) {
        distribute_croquettes();
    } else if (timeinfo.tm_hour == schedule->hour_4 && timeinfo.tm_min == schedule->minute_4) {
        distribute_croquettes();
    } else if (timeinfo.tm_hour == schedule->hour_5 && timeinfo.tm_min == schedule->minute_5) {
        distribute_croquettes();
    } else if (timeinfo.tm_hour == schedule->hour_6 && timeinfo.tm_min == schedule->minute_6) {
        distribute_croquettes();
    }
}

void distrib_time_check_callback(void *arg) {
    DistributionSchedule schedule;

    // Lire les horaires depuis la mémoire flash
    esp_err_t ret = read_distribution_schedule(&schedule);
    if (ret == ESP_OK) {
        check_and_distribute_croquettes(&schedule);
    } else {
        ESP_LOGE(TAG, "Erreur lors de la lecture des horaires depuis la mémoire flash");
    }
}

void periodic_time_check() {
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &distrib_time_check_callback,
        .arg = NULL,
        .name = "distrib_time_check"
    };
    esp_timer_handle_t periodic_timer;
    esp_timer_create(&periodic_timer_args, &periodic_timer);
    esp_timer_start_periodic(periodic_timer, 60 * 1000 * 1000); // Toutes les minutes
}

char *get_current_time_string() {
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

    char *result = malloc(strlen(strftime_buf) + 1);
    if (result != NULL) {
        strcpy(result, strftime_buf);
    }

    return result;
}

// Fonction utilitaire pour formater l'horaire en chaîne de caractères (HH:MM)
char *get_timer_string(int hour, int minute) {
    if (hour == 25 && minute == 61) {
        return strdup("inactif");
    }

    char *timer_string = (char *)malloc(6);  // 'HH:MM\0'
    if (timer_string == NULL) {
        ESP_LOGE(TAG, "Erreur d'allocation mémoire");
        return NULL;
    }

    snprintf(timer_string, 6, "%02d:%02d", hour, minute);

    return timer_string;
}

// Fonction pour obtenir une chaîne de caractères avec les horaires au format JSON
char *get_schedule_json() {
    DistributionSchedule schedule;
    esp_err_t ret = read_distribution_schedule(&schedule);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erreur lors de la lecture des horaires depuis la mémoire flash");
        return NULL;
    }

    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "timer1", get_timer_string(schedule.hour_1, schedule.minute_1));
    cJSON_AddStringToObject(root, "timer2", get_timer_string(schedule.hour_2, schedule.minute_2));
    cJSON_AddStringToObject(root, "timer3", get_timer_string(schedule.hour_3, schedule.minute_3));
    cJSON_AddStringToObject(root, "timer4", get_timer_string(schedule.hour_4, schedule.minute_4));
    cJSON_AddStringToObject(root, "timer5", get_timer_string(schedule.hour_5, schedule.minute_5));
    cJSON_AddStringToObject(root, "timer6", get_timer_string(schedule.hour_6, schedule.minute_6));

    char *json_string = cJSON_PrintUnformatted(root);

    // Libère la mémoire utilisée par l'objet cJSON
    cJSON_Delete(root);

    // Remplace "26:65" par "inactif" si présent dans la chaîne JSON
    char *inactive_string = strstr(json_string, "\"26:65\"");
    if (inactive_string != NULL) {
        memcpy(inactive_string, "\"inactif\"", 8);
    }

    return json_string;
}

// Fonction pour extraire et enregistrer les horaires depuis une chaîne JSON
void update_schedule_from_json(const char *json_string) {
    // Parse la chaîne JSON
    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        ESP_LOGE(TAG, "Erreur lors du parsing JSON");
        return;
    }

    // Crée une instance de DistributionSchedule
    DistributionSchedule new_schedule;

    // Initialise les heures à '25:61' par défaut
    new_schedule.hour_1 = new_schedule.hour_2 = new_schedule.hour_3 = new_schedule.hour_4 = new_schedule.hour_5 = new_schedule.hour_6 = 25;
    new_schedule.minute_1 = new_schedule.minute_2 = new_schedule.minute_3 = new_schedule.minute_4 = new_schedule.minute_5 = new_schedule.minute_6 = 61;

    // Attribue les horaires si présents dans la chaîne JSON
    for (int i = 1; i <= 6; i++) {
        char key[10];
        sprintf(key, "timer%d", i);

        cJSON *timer = cJSON_GetObjectItemCaseSensitive(root, key);
        if (timer != NULL) {
            int hour, minute;
            if (sscanf(timer->valuestring, "%d:%d", &hour, &minute) == 2) {
                switch (i) {
                    case 1:
                        new_schedule.hour_1 = hour;
                        new_schedule.minute_1 = minute;
                        break;
                    case 2:
                        new_schedule.hour_2 = hour;
                        new_schedule.minute_2 = minute;
                        break;
                    case 3:
                        new_schedule.hour_3 = hour;
                        new_schedule.minute_3 = minute;
                        break;
                    case 4:
                        new_schedule.hour_4 = hour;
                        new_schedule.minute_4 = minute;
                        break;
                    case 5:
                        new_schedule.hour_5 = hour;
                        new_schedule.minute_5 = minute;
                        break;
                    case 6:
                        new_schedule.hour_6 = hour;
                        new_schedule.minute_6 = minute;
                        break;
                    default:
                        break;
                }
            }
        }
    }

    // Libère la mémoire utilisée par l'objet cJSON
    cJSON_Delete(root);

    // Écrit les nouveaux horaires dans la mémoire flash NVS
    esp_err_t ret = write_distribution_schedule(&new_schedule);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Horaires mis à jour avec succès");
    } else {
        ESP_LOGE(TAG, "Erreur lors de la mise à jour des horaires : %s", esp_err_to_name(ret));
    }
    DistributionSchedule current_schedule;
    esp_err_t retu = read_distribution_schedule(&current_schedule);
    if (retu == ESP_OK) {
        ESP_LOGI(TAG, "Horaires actuels sauvegardés :");
        ESP_LOGI(TAG, "Timer 1 : %02d:%02d", current_schedule.hour_1, current_schedule.minute_1);
        ESP_LOGI(TAG, "Timer 2 : %02d:%02d", current_schedule.hour_2, current_schedule.minute_2);
        ESP_LOGI(TAG, "Timer 3 : %02d:%02d", current_schedule.hour_3, current_schedule.minute_3);
        ESP_LOGI(TAG, "Timer 4 : %02d:%02d", current_schedule.hour_4, current_schedule.minute_4);
        ESP_LOGI(TAG, "Timer 5 : %02d:%02d", current_schedule.hour_5, current_schedule.minute_5);
        ESP_LOGI(TAG, "Timer 6 : %02d:%02d", current_schedule.hour_6, current_schedule.minute_6);
    } else {
        ESP_LOGE(TAG, "Erreur lors de la lecture des horaires depuis la mémoire flash : %s", esp_err_to_name(ret));
    }
    send_schedule_to_mqtt();
}