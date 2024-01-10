#ifndef WEBCLIENT_H
#define WEBCLIENT_H

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"

// Définissez l'URL du serveur WebSocket
#define WS_SERVER_URL "ws://91.165.181.168:50000"

// Gestionnaire d'event websocket
void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

// Fonction pour envoyer et recevoir des données via WebSocket
void websocket_init();

#endif  // WEBCLIENT_H
