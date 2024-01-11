#ifndef TIME_H
#define TIME_H

#include "time.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "distrib/croquettes.h"

void initialize_sntp();
void wait_for_time();
void initialize_time();
void check_and_distribute_croquettes();
void periodic_task_callback(void* arg);
void initialize_periodic_task();
char* get_current_time_string();

#endif  // TIME_H