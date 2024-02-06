#ifndef TIME_H
#define TIME_H

#include "esp_system.h"

#define DISTRIBUTION_HOUR_1 14
#define DISTRIBUTION_MINUTE_1 46
#define DISTRIBUTION_HOUR_2 13
#define DISTRIBUTION_MINUTE_2 50
#define DISTRIBUTION_HOUR_3 20
#define DISTRIBUTION_MINUTE_3 30

// Structure pour stocker les horaires de distribution
typedef struct {
    int hour_1;
    int minute_1;
    int hour_2;
    int minute_2;
    int hour_3;
    int minute_3;
} DistributionSchedule;

// Fonction pour écrire les horaires dans la mémoire flash
esp_err_t write_distribution_schedule(const DistributionSchedule *schedule);

// Fonction pour lire les horaires depuis la mémoire flash
esp_err_t read_distribution_schedule(DistributionSchedule *schedule);

void initialize_sntp();

void wait_for_time();

void initialize_time();

void check_and_distribute_croquettes(const DistributionSchedule *schedule);

void periodic_task_callback(void *arg);

void initialize_periodic_task();

char *get_current_time_string();

#endif  // TIME_H
