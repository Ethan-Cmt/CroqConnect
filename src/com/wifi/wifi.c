#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "wifi.h"

#define WIFI_SSID "freebox_XZNEAJ"
#define WIFI_PASSWORD "99D654DC27E64AB6D7C4D019BC"

#define PHONE_SSID "iPhone de Ethan"
#define PHONE_PASSWORD "ethan93130"

#define CRETEIL_SSID "WATI-BOX"
#define CRETEIL_PASSWORD "SMJ2022_"

static const char *TAG = "wifi";

EventGroupHandle_t wifi_event_group;
bool ip_obtained = false;

static esp_err_t wifi_event_handler(void *ctx, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI(TAG, "Connecté au réseau Wi-Fi");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Déconnecté du réseau Wi-Fi");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_obtained = true;
        ESP_LOGI(TAG, "Adresse IP obtenue");
    }

    return ESP_OK;
}

void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());  // Initialiser esp_netif
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    esp_netif_create_default_wifi_sta();  // Créer l'interface par défaut pour le mode station

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CRETEIL_SSID,
            .password = CRETEIL_PASSWORD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

}