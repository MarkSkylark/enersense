#ifndef MQTT_APP_H
#define MQTT_APP_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Initializes MQTT client and starts background task that listens to data queue
void mqtt_app_start(QueueHandle_t data_queue);

#endif // MQTT_APP_H
