#ifndef WIFI_H
#define WIFI_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Initializes Wi-Fi connection and waits for IP address
//void wifi_init_sta(void);

// Updated: Takes a queue handle to pass data to the MQTT module when the network is ready
void wifi_init_sta(QueueHandle_t data_queue);

#endif // WIFI_H
