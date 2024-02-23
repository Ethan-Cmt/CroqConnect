#ifndef TIME_H
#define TIME_H

#include "esp_system.h"

// Structure pour stocker les horaires de distribution
typedef struct {
    int hour_1;
    int minute_1;
    int hour_2;
    int minute_2;
    int hour_3;
    int minute_3;
    int hour_4;
    int minute_4;
    int hour_5;
    int minute_5;
    int hour_6;
    int minute_6;
} DistributionSchedule;

// Fonction pour écrire les horaires dans la mémoire flash
esp_err_t write_distribution_schedule(const DistributionSchedule *schedule);

// Fonction pour lire les horaires depuis la mémoire flash
esp_err_t read_distribution_schedule(DistributionSchedule *schedule);

void initialize_sntp();

void wait_for_time();

void initialize_time();

void check_and_distribute_croquettes(const DistributionSchedule *schedule);

void distrib_time_check_callback(void *arg);

void periodic_time_check();

char *get_current_time_string();

void update_schedule_from_json(const char *json_string);

char *get_schedule_json();

char *get_timer_string();

void periodic_schedule_send();

#endif  // TIME_H
