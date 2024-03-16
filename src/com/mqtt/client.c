#include <mqtt_client.h>
#include "esp_log.h"
#include "esp_event.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "distrib/croquettes.h"
#include "distrib/fountain.h"
#include "camera/cam.h"
#include "time/time.h"
#include "quantity/hx711.h"
#include "quantity/portions.h"
#include "main.h"

#define MQTT_BROKER_ADDRESS "82.64.64.47"
#define MQTT_BROKER_PORT "55555"
#define MQTT_TESTER_ADRESS "test.mosquitto.org"
#define MQTT_TESTER_PORT "1883"


static const char *TAG = "mqtt";

bool mqtt_connected = false;

static esp_mqtt_client_handle_t client;

// Actions depending on MQTT messages recevied
void mqtt_process_received_data(const char *topic, int topic_len, const char *data, int data_len) { 
    ESP_LOGI(TAG, "Topic: %.*s, Data: %.*s", topic_len, topic, data_len, data);

    if (strncmp(topic, "distribution", topic_len) == 0) { // Instant distribution order
        distribute_croquettes(10);
    }
    if (strncmp(topic, "tasks/img_capture", topic_len) == 0) { // Not used
        if (strncmp(data, "on", data_len) == 0) {
            //send_mqtt_frame();
        } else if (strncmp(data, "off", data_len) == 0) {
            //controlImgCaptureTask(false);
        }
    }
    if (strncmp(topic, "timer/Settings", topic_len) == 0) { // Update schedule
        update_schedule_from_json(data);
    }
    if (strncmp(topic, "timer/Quantities", topic_len) == 0) { // Update quantities for scheduled distrib
        update_portions_from_json(data);
    }
    if (strncmp(topic, "quantity/taring", topic_len) == 0) { // Tare balance
        tare();
    }
    if (strncmp(topic, "fountain/set", topic_len) == 0) { // Enable/Disable water fountain
        if (strncmp(data, "on", data_len) == 0) {
            turn_fountain_on();
        } else if (strncmp(data, "off", data_len) == 0) {
            turn_fountain_off();
        }
    }
    if (strncmp(topic, "fountain/toggle", topic_len) == 0) { // Toggle fountain power
        toggle_fountain();
    }
}

void mqtt_publish_message(const char *topic, const char *message, int retain)
{
    esp_mqtt_client_publish(client, topic, message, 0, 1, retain);
    ESP_LOGI(TAG, "Message published on Topic : %s : %s", topic, message);
}

void mqtt_subscribe(const char *topic, int qos)
{
    esp_mqtt_client_subscribe(client, (char *)topic, qos);
    ESP_LOGI(TAG, "Subscribting to MQTT Topic : %s", topic);
}

void mqtt_unsubscribe(const char *topic)
{
    esp_mqtt_client_unsubscribe(client, topic);
    ESP_LOGI(TAG, "Unsubscribing to MQTT Topic : %s", topic);
}

// MQTT Event Handler
static esp_err_t mqtt_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t) event_handler_arg;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected to MQTT Broker");
            mqtt_connected = true;
            mqtt_subscribe("distribution", 1);
            mqtt_subscribe("tasks/img_capture", 1);
            mqtt_subscribe("timer/Settings", 1);
            mqtt_subscribe("timer/Quantities", 1);
            mqtt_subscribe("quantity/taring", 1);
            mqtt_subscribe("fountain/set", 1);
            mqtt_subscribe("fountain/toggle", 1);
            xSemaphoreGive(mqttConnectedSemaphore);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected from MQTT Broker");
            mqtt_connected = false;
            //vTaskDelay(pdMS_TO_TICKS(5000));
            //esp_mqtt_client_reconnect(client);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "Successfull MQTT subscription");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "Successfull MQTT publish");
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT message received");
            mqtt_process_received_data(event->topic, event->topic_len, event->data, event->data_len);
            break;
        default:
            break;
    }

    return ESP_OK;
}

// Main MQTT client initalizer
void mqtt_app_start(void)
{
    ESP_LOGI(TAG, "Init MQTT conifg...");
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
            .username = "proto1", // MQTT username
            .client_id = "Distributor", // MQTT client identifier, set to NULL for default client id
            .set_null_client_id = false, // Selects a NULL client id
            .authentication = {
                .password = "proto1", // MQTT password
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
            .reconnect_timeout_ms = 2000, // Reconnect to the broker after this value in milliseconds
            .timeout_ms = 10000, // Abort network operation if not completed after this value in milliseconds
            .refresh_connection_after_ms = 65000, // Refresh connection after this value in milliseconds
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

    ESP_LOGI(TAG, "MQTT Client Init...");
    client = esp_mqtt_client_init(&mqtt_cfg);

    ESP_LOGI(TAG, "Loading MQTT Event Handler...");
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, &mqtt_event_handler, client);

    ESP_LOGI(TAG, "Starting MQTT client...");
    esp_mqtt_client_start(client);

    ESP_LOGI(TAG, "MQTT Start Done. Waiting for connection...");

}

void send_image_data(uint8_t *image_data, size_t image_size)
{
    if (xSemaphoreTake(imageUploadSemaphore, portMAX_DELAY) == pdTRUE)
    {
        esp_err_t result = esp_mqtt_client_publish(client, "image", (char *)image_data, image_size, 0, 0);
        
        //xSemaphoreGive(imageUploadSemaphore);

        if (result != ESP_OK) {
            ESP_LOGI(TAG, "Done publishing image : %d", result);
            xSemaphoreGive(imageUploadSemaphore);
        }
        else {
            ESP_LOGI(TAG, "Done publishing image : %d", result);
            xSemaphoreGive(imageUploadSemaphore); 
        }
    }
    else
    {
        // La tâche n'a pas pu acquérir le sémaphore, gestion de l'erreur ici
        ESP_LOGE(TAG, "Impossible d'acquérir le sémaphore pour l'envoi d'image");
    }
}

void send_schedule_to_mqtt() {
    char *schedule_string = get_schedule_json();
    esp_mqtt_client_publish(client, "timer/CurrentTimers", schedule_string, 0, 1, 0);
}

void send_portions_to_mqtt() {
    char *portions_string = get_portions_json();
    esp_mqtt_client_publish(client, "timer/CurrentQuantities", portions_string, 0, 1, 0);
}

void send_quantity_to_mqtt() {
    char quantity_string[5];
    snprintf(quantity_string, sizeof(quantity_string), "%" PRIi32, get_quantity());
    esp_mqtt_client_publish(client, "quantity/Current", quantity_string, 0, 0, 0);
}