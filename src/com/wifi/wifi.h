#ifndef WIFI_H
#define WIFI_H

#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

extern bool ip_obtained;

void wifi_init_softap(void);
void wifi_init_sta(void);

#endif  // WIFI_H
