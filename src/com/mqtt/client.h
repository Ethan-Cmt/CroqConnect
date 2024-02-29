#ifndef CLIENT_H
#define CLIENT_H

#include <mqtt_client.h>

void mqtt_app_start(void);
void mqtt_publish_message(const char *topic, const char *message, int retain);
void mqtt_task(void *pvParameters);
void send_image_data(uint8_t *image_data, size_t image_size);
void send_schedule_to_mqtt();
void send_quantity_to_mqtt();
void send_portions_to_mqtt();
extern bool mqtt_connected;


#endif  // CLIENT_H
