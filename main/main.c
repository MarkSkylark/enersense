#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "esp_log.h"

// include module calls
#include "sensor/dht22.h"
#include "sensor/dht22.h"
#include "network/wifi.h"
#include "mqtt/mqtt_app.h"

static const char *TAG = "MAIN_APP";
static QueueHandle_t sensor_data_queue = NULL;

void sensor_task(void *pvParameters) {
    dht22_data_t sensor_data;
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000)); // DHT22 read interval (5 seconds)
        
        if (dht22_read(&sensor_data) == ESP_OK) {
            ESP_LOGI(TAG, "Sensor read: T=%.1f C, H=%.1f %%", sensor_data.temperature, sensor_data.humidity);
            
            // Send data to MQTT task queue (do not wait if queue is full)
            if (xQueueSend(sensor_data_queue, &sensor_data, 0) != pdPASS) {
                ESP_LOGW(TAG, "Queue full! Data could not be sent.");
            }
        } else {
            ESP_LOGE(TAG, "Sensor read failed.");
        }
    }
}

void app_main(void) {
    // 1. Initialize NVS (Non-Volatile Storage) - required for Wi-Fi functionality
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Initializing modular system...");

    // 2. Create FreeRTOS queue for data transfer between modules (can hold 5 measurements at a time)
    sensor_data_queue = xQueueCreate(5, sizeof(dht22_data_t));
    if (sensor_data_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queue!");
        return;
    }

    // 3. Initialize hardware and network
    dht22_init();
    wifi_init_sta(sensor_data_queue);

    // 4. Start sensor reading task
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
}

