#ifndef DHT22_H
#define DHT22_H

#include "esp_err.h"

typedef struct {
    float temperature;
    float humidity;
} dht22_data_t;

// Initializes the DHT22 sensor.
void dht22_init(void);

// Reads temperature and humidity from the DHT22 sensor.
esp_err_t dht22_read(dht22_data_t *data);

#endif // DHT22_H