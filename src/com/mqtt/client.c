#include <mqtt_client.h>
#include "esp_log.h"
#include "esp_event.h"

#include "distrib/croquettes.h"
#include "camera/cam.h"
#include "main.h"

#define MQTT_BROKER_ADDRESS "91.165.181.168"
#define MQTT_BROKER_PORT "55555"

static const char *TAG = "mqtt";

static esp_mqtt_client_handle_t client;

void mqtt_process_received_data(const char *topic, int topic_len, const char *data, int data_len) {
    ESP_LOGI(TAG, "Topic: %.*s, Data: %.*s", topic_len, topic, data_len, data);

    // Vérifier si le message est reçu sur le topic 'distribution'
    if (strncmp(topic, "distribution", topic_len) == 0) {
        distribute_croquettes();
    }
    // Vérifier si le message est reçu sur le topic 'tasks/img_capture'
    if (strncmp(topic, "tasks/img_capture", topic_len) == 0) {
        // Démarrer ou arrêter la tâche en fonction de la valeur du message
        if (strncmp(data, "on", data_len) == 0) {
            controlImgCaptureTask(true);
        } else if (strncmp(data, "off", data_len) == 0) {
            controlImgCaptureTask(false);
        }
    }
}

void mqtt_publish_message(const char *topic, const char *message, int retain)
{
    esp_mqtt_client_publish(client, topic, message, 0, 1, retain);
    ESP_LOGI(TAG, "Message publié sur le topic %s : %s", topic, message);
}

void mqtt_subscribe(const char *topic, int qos)
{
    esp_mqtt_client_subscribe(client, (char *)topic, qos);
    ESP_LOGI(TAG, "Abonnement au topic MQTT : %s", topic);
}

void mqtt_unsubscribe(const char *topic)
{
    esp_mqtt_client_unsubscribe(client, topic);
    ESP_LOGI(TAG, "Désabonnement du topic MQTT : %s", topic);
}

static esp_err_t mqtt_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t) event_handler_arg;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connecté au broker MQTT");
            mqtt_subscribe("distribution", 1);
            mqtt_subscribe("tasks/img_capture", 1);
            xSemaphoreGive(mqttConnectedSemaphore);
            xSemaphoreGive(imageUploadSemaphore);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Déconnecté du broker MQTT");
            esp_mqtt_client_reconnect(client);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "Abonnement réussi à un sujet MQTT");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "Message MQTT publié avec succès");
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Message MQTT reçu");
            mqtt_process_received_data(event->topic, event->topic_len, event->data, event->data_len);
            break;
        default:
            break;
    }

    return ESP_OK;
}

void mqtt_app_start(void)
{
    ESP_LOGI(TAG, "Initialisation de la configuration MQTT");
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .uri = "mqtt://" MQTT_BROKER_ADDRESS ":" MQTT_BROKER_PORT, //if uri is set, leave adress info NULL
                .hostname = NULL, // Set the hostname if uri is not usedstatic const char *TAG = "mqtt";
                .transport = MQTT_TRANSPORT_UNKNOWN, // MQTT_TRANSPORT_OVER_TCP or MQTT_TRANSPORT_OVER_SSL or UNKNOWN
                .path = NULL, // Path in the URI
                .port = 0, // Set the port if uri is not used
            },
        },
        .credentials = {
            .username = NULL, // MQTT username
            .client_id = "ethan", // MQTT client identifier, set to NULL for default client id
            .set_null_client_id = false, // Selects a NULL client id
            .authentication = {
                .password = "pswrd", // MQTT password
                .certificate = NULL, // Certificate for SSL mutual authentication
                .certificate_len = 0, // Length of the buffer pointed to by certificate
                .key = NULL, // Private key for SSL mutual authentication
                .key_len = 0, // Length of the buffer pointed to by key
                .key_password = NULL, // Client key decryption password
                .key_password_len = 0, // Length of the password pointed to by key_password
                .use_secure_element = false, // Enable secure element for SSL connection
                .ds_data = NULL, // Carrier of handle for digital signature parameters
            },
        },
        .session = {
            .last_will = {
                .topic = NULL, // LWT message topic
                .msg = NULL, // LWT message
                .msg_len = 0, // LWT message length
                .qos = 0, // LWT message QoS
                .retain = 0, // LWT retained message flag
            },
            .disable_clean_session = false, // MQTT clean session
            .keepalive = 120, // MQTT keepalive in seconds
            .disable_keepalive = false, // Disable keep-alive mechanism
            .protocol_ver = MQTT_PROTOCOL_V_3_1_1, // MQTT protocol version
            .message_retransmit_timeout = 5, // Timeout for retransmitting of a failed packet
        },
        .network = {
            .reconnect_timeout_ms = 10000, // Reconnect to the broker after this value in milliseconds
            .timeout_ms = 10000, // Abort network operation if not completed after this value in milliseconds
            .refresh_connection_after_ms = 60000, // Refresh connection after this value in milliseconds
            .disable_auto_reconnect = false, // Client will reconnect to the server (when errors/disconnect)
            .transport = NULL, // Custom transport handle to use, NULL for default
            .if_name = NULL, // The name of the interface for data to go through
        },
        .task = {
            .priority = 5, // MQTT task priority
            .stack_size = 4096, // MQTT task stack size
        },
        .buffer = {
            .size = 2048, // Size of MQTT send/receive buffer
            .out_size = 0, // Size of MQTT output buffer, defaults to the size defined by buffer_size
        },
        .outbox = {
            .limit = 0, // Size limit for the outbox in bytes
        },
    };

    ESP_LOGI(TAG, "Initialisation du client MQTT");
    client = esp_mqtt_client_init(&mqtt_cfg);

    ESP_LOGI(TAG, "Enregistrement du gestionnaire d'événements MQTT");
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, &mqtt_event_handler, client);

    ESP_LOGI(TAG, "Démarrage du client MQTT");
    esp_mqtt_client_start(client);

    ESP_LOGI(TAG, "Fonction mqtt_app_start terminée");

}

void send_image_data(uint8_t *image_data, size_t image_size)
{
    if (xSemaphoreTake(imageUploadSemaphore, portMAX_DELAY) == pdTRUE) {
        esp_err_t result = esp_mqtt_client_publish(client, "image", (char *)image_data, image_size, 1, 0);
        if (result != ESP_OK) {
            ESP_LOGI(TAG, "Fin du mqtt_publish : %d", result);
        }
        xSemaphoreGive(imageUploadSemaphore);
    } else {
        ESP_LOGE(TAG, "Impossible d'obtenir le sémaphore d'envoi d'image");
    }
}
