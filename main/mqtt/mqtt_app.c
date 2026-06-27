#include "mqtt_app.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "project_config.h"
#include "sensor/dht22.h" // Tarvitaan dht22_data_t rakenne

static const char *TAG = "MQTT";
static esp_mqtt_client_handle_t client = NULL;
// KORJAUS: Lisätty volatile, jotta FreeRTOS-tehtävät näkevät muuttujan tilan livenä
static volatile bool mqtt_connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t event_base, 
                               int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Yhdistetty brokerille.");
            mqtt_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT Yhteys katkesi.");
            mqtt_connected = false;
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT Tapahtumavirhe! Koodi: %d", event->error_handle->error_type);
            break;
        default:
            break;
    }
}

// Taustatehtävä joka odottaa sensoridataa jonosta ja julkaisee sen MQTT:llä
static void mqtt_worker_task(void *pvParameters) {
    QueueHandle_t data_queue = (QueueHandle_t)pvParameters;
    dht22_data_t received_data;
    
    // KORJAUS: Lisätty [64] jotta puskuri on oikeasti taulukko
    char payload[64]; 

    while (1) {
        if (xQueueReceive(data_queue, &received_data, portMAX_DELAY) == pdPASS) {
            if (mqtt_connected) {
                // Luodaan JSON-muotoinen teksti puskuriin
                snprintf(payload, sizeof(payload), 
                         "{\"temperature\": %.1f, \"humidity\": %.1f}", 
                         received_data.temperature, received_data.humidity);

                // Julkaistaan viesti
                int msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC_DATA, payload, 0, 1, 0);
                ESP_LOGI(TAG, "Julkaistu aiheeseen %s, msg_id=%d, data=%s", MQTT_TOPIC_DATA, msg_id, payload);
            } else {
                ESP_LOGW(TAG, "Data vastaanotettu jonosta, mutta MQTT ei ole yhdistetty. Ohitetaan.");
            }
        }
    }
}


void mqtt_app_start(QueueHandle_t data_queue) {
    // ESP-IDF v5.x mukainen moderni konfiguraatio
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .uri = MQTT_BROKER_URI, // Lukee osoitteen project_config.h tiedostosta
            },
        },
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    
    // Varmistetaan, että aiemmin korjattu funktion nimi on tässä muodossa:
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    
    esp_mqtt_client_start(client);

    // Käynnistetään FreeRTOS-tehtävä käsittelemään jonoa
    xTaskCreate(mqtt_worker_task, "mqtt_worker_task", 4096, (void*)data_queue, 5, NULL);
}

