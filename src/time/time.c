#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "nvs_flash.h" // Ajout de l'inclusion de la bibliothèque NVS
#include "distrib/croquettes.h"

#include "time.h"

static const char *TAG = "time sync";

#define DISTRIBUTION_SCHEDULE_NAMESPACE "schedule"

#define DISTRIBUTION_HOUR_1 13
#define DISTRIBUTION_MINUTE_1 50
#define DISTRIBUTION_HOUR_2 13
#define DISTRIBUTION_MINUTE_2 50
#define DISTRIBUTION_HOUR_3 20
#define DISTRIBUTION_MINUTE_3 30

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
    }
}

void periodic_task_callback(void *arg) {
    DistributionSchedule schedule;

    // Lire les horaires depuis la mémoire flash
    esp_err_t ret = read_distribution_schedule(&schedule);
    if (ret == ESP_OK) {
        check_and_distribute_croquettes(&schedule);
    } else {
        ESP_LOGE(TAG, "Erreur lors de la lecture des horaires depuis la mémoire flash");
    }
}

void initialize_periodic_task() {
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &periodic_task_callback,
        .arg = NULL,
        .name = "periodic_task"
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
