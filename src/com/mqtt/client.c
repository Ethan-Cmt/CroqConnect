#include <mqtt_client.h>
#include "esp_log.h"
#include "esp_event.h"

#define MQTT_BROKER_ADDRESS "91.165.181.168"
#define MQTT_BROKER_PORT "55555"

static const char *TAG = "mqtt";

static esp_mqtt_client_handle_t client;

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    // Gestionnaire d'événements pour les événements MQTT
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Connecté au broker MQTT");
        // Souscrire à des sujets, publier, etc. ici
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Déconnecté du broker MQTT");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "Abonnement réussi à un sujet MQTT");
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "Message MQTT publié avec succès");
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "Message MQTT reçu");
        // Gérer les messages MQTT entrants ici
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
            .client_id = NULL, // MQTT client identifier, set to NULL for default client id
            .set_null_client_id = false, // Selects a NULL client id
            .authentication = {
                .password = NULL, // MQTT password
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
            .size = 512, // Size of MQTT send/receive buffer
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

void mqtt_publish_message(const char *topic, const char *message)
{
    esp_mqtt_client_publish(client, topic, message, 0, 1, 0);
    ESP_LOGI(TAG, "Message publié sur le topic %s : %s", topic, message);
}