#ifndef CLIENT_H
#define CLIENT_H

#include <mqtt_client.h>

void mqtt_app_start(void);
void mqtt_publish_message(const char *topic, const char *message, int retain);
void mqtt_task(void *pvParameters);


#endif  // CLIENT_H
