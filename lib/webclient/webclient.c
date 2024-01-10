#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_websocket_client.h"

// Remplacez ces valeurs par les informations de votre réseau Wi-Fi
#define WIFI_SSID "freebox_XZNEAJ"
#define WIFI_PASSWORD "pswd"

// Remplacez ces valeurs par les informations de votre serveur WebSocket
#define WS_SERVER_URL "ws://91.165.181.168:50000"

static const char *TAG = "websocket_client";
static esp_websocket_client_handle_t client;

// Les événements Wi-Fi qui déclencheront le callback
const int CONNECTED_BIT = BIT0;

// Gestionnaire d'événements WebSocket
void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connecté au serveur WebSocket");
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Déconnecté du serveur WebSocket");
            break;
        case WEBSOCKET_EVENT_DATA:
            ESP_LOGI(TAG, "Données reçues du serveur WebSocket: %.*s", data->data_len, (char *)data->data_ptr);
            // Ajoutez votre traitement des données reçues ici
            break;
        default:
            break;
    }
}

// Fonction pour configurer le client WebSocket
void websocket_init() {
    esp_websocket_client_config_t ws_cfg = {
        .uri = WS_SERVER_URL,
    };

    client = esp_websocket_client_init(&ws_cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, client);
    esp_websocket_client_start(client);
}