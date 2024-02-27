#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "wifi.h"

#define WIFI_SSID "freebox_XZNEAJ"
#define WIFI_PASSWORD "99D654DC27E64AB6D7C4D019BC"

#define PHONE_SSID "iPhone de Ethan"
#define PHONE_PASSWORD "ethan93130"

#define CRETEIL_SSID "WATI-BOX"
#define CRETEIL_PASSWORD "SMJ2022_"

#define AP_SSID "Croq'Connect"
#define AP_PASS "password!"

#define MAX_WIFI_RESULTS 10 // Max number of proposed networks from scan

// Wifi Scan data
typedef struct {
    char ssid[32];
    int rssi;
} wifi_info_t; 

// Wifi credentials data
typedef struct {
    char ssid[32];
    char password[64];
} wifi_credentials_t;

wifi_info_t wifi_results[MAX_WIFI_RESULTS];
int num_wifi_results = 0;

static const char *TAG = "wifi";

EventGroupHandle_t wifi_event_group;

// Constant for STA Mode
bool ip_obtained = false; 
static int reconnect_attempts = 0;
#define MAX_RECONNECT_ATTEMPTS 30
#define RECONNECT_INTERVAL_MS 1000

static esp_err_t wifi_event_handler(void *ctx, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "Connected to Wifi");
        reconnect_attempts = 0;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected from Wifi");
        if (reconnect_attempts < MAX_RECONNECT_ATTEMPTS) {
            vTaskDelay(pdMS_TO_TICKS(RECONNECT_INTERVAL_MS));
            esp_wifi_connect();
            ESP_LOGI(TAG, "Trying to reconnect... #%d", ++reconnect_attempts);
        } else {
            ESP_LOGE(TAG, "Wifi connection failed after %d attempts", MAX_RECONNECT_ATTEMPTS);
            reconnect_attempts = 0;
            wifi_init_softap();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_obtained = true;
        ESP_LOGI(TAG, "IP Adress obtained");
        reconnect_attempts = 0;
    }

    return ESP_OK;
}

// Get Wifi Credentials saved in flash memory
esp_err_t load_wifi_credentials(wifi_credentials_t *wifi_credentials) {
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("wifi_data", NVS_READONLY, &nvs_handle);
    if (ret == ESP_OK) {
        size_t size = sizeof(wifi_credentials_t);
        ret = nvs_get_blob(nvs_handle, "wifi_data", wifi_credentials, &size);
        nvs_close(nvs_handle);
    }
    return ret;
}

// Save Wifi Credentials in flash memory
esp_err_t save_wifi_credentials(const char *ssid, const char *password) {
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("wifi_data", NVS_READWRITE, &nvs_handle);

    if (ret == ESP_OK) {
        wifi_credentials_t wifi_credentials;
        memset(&wifi_credentials, 0, sizeof(wifi_credentials_t)); // Init struct w/ 0s

        strncpy(wifi_credentials.ssid, ssid, sizeof(wifi_credentials.ssid) - 1);
        wifi_credentials.ssid[sizeof(wifi_credentials.ssid) - 1] = '\0';  // Add NULL char

        strncpy(wifi_credentials.password, password, sizeof(wifi_credentials.password) - 1);
        wifi_credentials.password[sizeof(wifi_credentials.password) - 1] = '\0';  // Add NULL char

        ESP_LOGI(TAG, "SSID before saving is %s", wifi_credentials.ssid);
        ESP_LOGI(TAG, "PASSWORD before saving is %s", wifi_credentials.password);

        ret = nvs_set_blob(nvs_handle, "wifi_data", &wifi_credentials, sizeof(wifi_credentials));
        if (ret == ESP_OK) {
            ret = nvs_commit(nvs_handle);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Wifi Credentials saved");
            } else {
                ESP_LOGE(TAG, "Failed to commit NVS. Error code: %d", ret);
            }
        } else {
            ESP_LOGE(TAG, "Failed to set blob in NVS. Error code: %d", ret);
        }

        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "Failed to open NVS. Error code: %d", ret);
    }

    return ret;
}

void scan_wifi_networks() {
    ESP_LOGI(TAG, "Scanning WiFi networks...");

    wifi_scan_config_t scan_config = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_PASSIVE,
        .scan_time.passive = 500,
    };

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
    
    uint16_t ap_num;
    esp_wifi_scan_get_ap_num(&ap_num);

    if (ap_num == 0) {
        ESP_LOGI(TAG, "No WiFi networks found.");
        return;
    }

    wifi_ap_record_t *ap_records = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * ap_num);
    if (ap_records == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for AP records.");
        return;
    }

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_records));

    int num_wifi_results_before_filtering = 0;

    for (int i = 0; i < ap_num && i < MAX_WIFI_RESULTS; i++) {
        char ssid_str[sizeof(ap_records[i].ssid) + 1];
        memcpy(ssid_str, ap_records[i].ssid, sizeof(ap_records[i].ssid));
        ssid_str[sizeof(ap_records[i].ssid)] = '\0';

        // Special char filter for html compatibilty
        char sanitized_ssid[sizeof(ap_records[i].ssid) + 1];
        size_t j = 0;
        for (size_t k = 0; k < sizeof(ap_records[i].ssid); k++) {
            // Ignores ASCII chars under 32 and over 126
            if (ssid_str[k] >= 32 && ssid_str[k] <= 126) {
                sanitized_ssid[j++] = ssid_str[k];
            }
        }
        sanitized_ssid[j] = '\0';

        // SSID size check (No SSID over 31 chars)
        if (strlen(sanitized_ssid) > 31) {
            continue;
        }

        strncpy(wifi_results[num_wifi_results_before_filtering].ssid, sanitized_ssid, sizeof(wifi_results[num_wifi_results_before_filtering].ssid));
        wifi_results[num_wifi_results_before_filtering].rssi = ap_records[i].rssi;
        ESP_LOGI(TAG, "WiFi Network: SSID=%s, RSSI=%d", wifi_results[num_wifi_results_before_filtering].ssid, wifi_results[num_wifi_results_before_filtering].rssi);
        
        num_wifi_results_before_filtering++;
    }

    num_wifi_results = num_wifi_results_before_filtering;

    free(ap_records);

    ESP_LOGI(TAG, "Scan completed. Number of WiFi networks: %d", num_wifi_results);
}

// Get Wifi credentials HTTP Handler
esp_err_t main_wifi_cred_get_handler(httpd_req_t *req) {
    char buffer[1024];
    int total_len = 0;

    total_len += snprintf(buffer + total_len, sizeof(buffer) - total_len, "<!DOCTYPE html><html><head>"
                        "<meta charset=\"UTF-8\">"
                        "<style>"
                        "body { font-size: 20px; }"
                        "ul { list-style-type: none; padding: 0; }"
                        "li { margin-bottom: 10px; cursor: pointer; }"
                        "input { font-size: 16px; }"
                        "</style></head><body>"
                        "<h1>Veuillez saisir vos identifiants wifi:</h1>"
                        "<form method=\"POST\" action=\"/connect\">"
                        "<select name=\"selected_ssid\">");

    for (int i = 0; i < num_wifi_results; i++) {
        total_len += snprintf(buffer + total_len, sizeof(buffer) - total_len,
                              "<option value=\"%s\">%s</option>", wifi_results[i].ssid, wifi_results[i].ssid);
    }

    total_len += snprintf(buffer + total_len, sizeof(buffer) - total_len,
                          "</select>"
                          "<br>"
                          "<label for=\"password\">Mot de passe:</label>"
                          "<br>"
                          "<input type=\"password\" id=\"password\" name=\"password\" placeholder=\"Mot de passe\">"
                          "<br>"
                          "<input type=\"submit\" value=\"Connecter\">"
                          "</form></body></html>");


    vTaskDelay(1000 / portTICK_PERIOD_MS);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, buffer, total_len);

    return ESP_OK;
}

httpd_uri_t get_id_uri = {
    .uri = "/hello",
    .method = HTTP_GET,
    .handler = main_wifi_cred_get_handler,
    .user_ctx = NULL
};

// Save Wifi credentials HTTP Handler
esp_err_t connect_post_handler(httpd_req_t *req) {
    char ssid[32];
    char password[64];

    #define MAX_POST_DATA_SIZE 128
    char post_data[MAX_POST_DATA_SIZE];

    // Récupérer les données POST sous forme de chaîne
    size_t post_data_len = httpd_req_recv(req, post_data, MAX_POST_DATA_SIZE);
    if (post_data_len <= 0) {
        ESP_LOGE(TAG, "Error while receiving POST data");
        return ESP_FAIL;
    }

    post_data[post_data_len] = '\0';

    // Extracting wifi credentials from Form
    if (httpd_query_key_value(post_data, "selected_ssid", ssid, sizeof(ssid)) != ESP_OK ||
        httpd_query_key_value(post_data, "password", password, sizeof(password)) != ESP_OK) {
        ESP_LOGE(TAG, "Error while extracting POST data");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Received SSID: %s", ssid);
    ESP_LOGI(TAG, "Received Password : %s", password);

    esp_err_t ret = save_wifi_credentials(ssid, password);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error while saving Wifi credentials in flash memory");
        return ret;
    }

    httpd_resp_send(req, "Saving Wifi Credentials in flash memory...", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "Restarting ESP32...");
    esp_restart();

    return ESP_OK;
}

httpd_uri_t connect_uri = {
    .uri = "/connect",
    .method = HTTP_POST,
    .handler = connect_post_handler,
    .user_ctx = NULL
};

void wifi_init_softap() {
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    //esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .password = AP_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        }
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WiFi AP initialized, SSID: %s", AP_SSID);
    scan_wifi_networks();
    vTaskDelay(3400 / portTICK_PERIOD_MS);

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 4096 * 2;
    ESP_ERROR_CHECK(httpd_start(&server, &config));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &get_id_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &connect_uri));


    ESP_LOGI(TAG, "Web Server Initialized.");
}

void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());  
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    esp_netif_create_default_wifi_sta();  

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_credentials_t wifi_credentials;

    if (load_wifi_credentials(&wifi_credentials) == ESP_OK) {
        wifi_config_t wifi_sta_config = {
            .sta = {
                .ssid = {0},
                .password = {0},
            },
        };

        snprintf((char*)wifi_sta_config.sta.ssid, sizeof(wifi_sta_config.sta.ssid), "%s", wifi_credentials.ssid);
        snprintf((char*)wifi_sta_config.sta.password, sizeof(wifi_sta_config.sta.password), "%s", wifi_credentials.password);

        wifi_sta_config.sta.ssid[sizeof(wifi_sta_config.sta.ssid) - 1] = '\0';
        wifi_sta_config.sta.password[sizeof(wifi_sta_config.sta.password) - 1] = '\0';

        ESP_LOGI(TAG, "SSID from flash : %s", wifi_sta_config.sta.ssid);
        ESP_LOGI(TAG, "password from flash : %s", wifi_sta_config.sta.password);

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
        ESP_ERROR_CHECK(esp_wifi_start());
    } else {
        ESP_LOGE(TAG, "Failed to load WiFi credentials. Starting in AP mode.");
        wifi_init_softap();
    }
}
