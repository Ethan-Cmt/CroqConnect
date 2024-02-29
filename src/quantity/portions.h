#ifndef PORTIONS_H
#define PORTIONS_H

#include "esp_system.h"

typedef struct {
    int portion_1;
    int portion_2;
    int portion_3;
    int portion_4;
    int portion_5;
    int portion_6;
} TimerPortions;

esp_err_t write_distribution_portions(const TimerPortions *portions);

esp_err_t read_distribution_portions(TimerPortions *portions);

TimerPortions get_distrib_quantity();

void update_portions_from_json(const char *json_string);

char *get_portions_json();

void periodic_portions_send();

#endif  // PORTIONS_H