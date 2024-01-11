#include "time.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "distrib/croquettes.h"

static const char *TAG = "time sync";

#define DISTRIBUTION_HOUR_1 10
#define DISTRIBUTION_HOUR_2 16
#define DISTRIBUTION_MINUTE_2 30
#define DISTRIBUTION_HOUR_3 20
#define DISTRIBUTION_MINUTE_3 30

void initialize_sntp() {
    ESP_LOGI(TAG, "Initialisation du client SNTP...");

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");

    esp_sntp_init();
}

void wait_for_time() {
    ESP_LOGI(TAG, "En attente de la synchronisation de l'heure...");
    time_t now = 0;
    struct tm timeinfo = { 0 };

    while (timeinfo.tm_year < (2024 - 1900)) {
        time(&now);
        localtime_r(&now, &timeinfo);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Heure synchronisée avec succès");
}

void initialize_time() {
    setenv("TZ", "UTC", 1); // Utilisation du temps universel coordonné (UTC)
    tzset();

    time_t now;
    struct tm timeinfo;

    time(&now);
    gmtime_r(&now, &timeinfo);

    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "Heure actuelle (UTC) : %s", strftime_buf);

    // Ajuster le fuseau horaire en fonction du mois
    if (timeinfo.tm_mon >= 3 && timeinfo.tm_mon < 10) {
        setenv("TZ", "CEST-2", 1); // Heure d'été (Central European Summer Time)
    } else {
        setenv("TZ", "CET-1", 1); // Heure d'hiver (Central European Time)
    }

    tzset();

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "Heure actuelle (Fuseau horaire français) : %s", strftime_buf);
}

void check_and_distribute_croquettes() {
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_hour == DISTRIBUTION_HOUR_1 && timeinfo.tm_min == 0) {
        distribute_croquettes();
    } else if (timeinfo.tm_hour == DISTRIBUTION_HOUR_2 && timeinfo.tm_min == DISTRIBUTION_MINUTE_2) {
        distribute_croquettes();
    } else if (timeinfo.tm_hour == DISTRIBUTION_HOUR_3 && timeinfo.tm_min == DISTRIBUTION_MINUTE_3) {
        distribute_croquettes();
    }
}

void periodic_task_callback(void* arg) {
    check_and_distribute_croquettes();
}

void initialize_periodic_task() {
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &periodic_task_callback,
        .arg = NULL,
        .name = "periodic_task"
    };
    esp_timer_handle_t periodic_timer;
    esp_timer_create(&periodic_timer_args, &periodic_timer);
    esp_timer_start_periodic(periodic_timer, 60 * 1000 * 1000);  // Toutes les minutes
}

char* get_current_time_string() {
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

    // Allouer dynamiquement de la mémoire pour stocker la chaîne et la retourner
    char* result = malloc(strlen(strftime_buf) + 1);
    if (result != NULL) {
        strcpy(result, strftime_buf);
    }

    return result;
    // WARNING : Penser à libérer la mémoire allouée avec 'free(current_time_str);'
}