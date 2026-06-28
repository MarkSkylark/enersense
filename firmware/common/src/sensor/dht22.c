#include "dht22.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "config/project_config.h"

static void dht22_start_signal(void) {
    gpio_set_direction(DHT22_GPIO_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT22_GPIO_PIN, 0);
    ets_delay_us(20000); // Vedetään linja alas vähintään 18ms
    gpio_set_level(DHT22_GPIO_PIN, 1);
    ets_delay_us(30);    // Vedetään ylös 20-40ms
    gpio_set_direction(DHT22_GPIO_PIN, GPIO_MODE_INPUT);
}

static esp_err_t dht22_check_response(void) {
    ets_delay_us(40);
    if (gpio_get_level(DHT22_GPIO_PIN) == 0) {
        ets_delay_us(80);
        if (gpio_get_level(DHT22_GPIO_PIN) == 1) {
            ets_delay_us(40);
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

static uint8_t dht22_read_byte(void) {
    uint8_t data = 0;
    
    for (int i = 0; i < 8; i++) {
        int timeout_counter = 0;
        
        // 1. Odotetaan, että signaali nousee korkeaksi (bitti alkaa)
        while (gpio_get_level(DHT22_GPIO_PIN) == 0) {
            ets_delay_us(1);
            timeout_counter++;
            if (timeout_counter > 1000) return 0; // Jos odotettu yli 1ms, katkaistaan silmukka!
        }
        
        ets_delay_us(40); // Odotetaan ja katsotaan bitin pituus
        
        // 2. Jos linja on yhä ylhäällä 40 mikrosekunnin jälkeen, bitti on '1'
        if (gpio_get_level(DHT22_GPIO_PIN) == 1) {
            data |= (1 << (7 - i));
            
            timeout_counter = 0;
            // Odotetaan, että signaali laskee taas alas seuraavaa bittiä varten
            while (gpio_get_level(DHT22_GPIO_PIN) == 1) {
                ets_delay_us(1);
                timeout_counter++;
                if (timeout_counter > 1000) return 0; // Turvasuoja jumiutumista vastaan
            }
        }
    }
    return data;
}


void dht22_init(void) {
    gpio_reset_pin(DHT22_GPIO_PIN);
    gpio_set_pull_mode(DHT22_GPIO_PIN, GPIO_PULLUP_ONLY);
}

esp_err_t dht22_read(dht22_data_t *data) {
    uint8_t raw_data[5] = {0};
    
    dht22_start_signal();
    if (dht22_check_response() != ESP_OK) {
        return ESP_ERR_TIMEOUT;
    }
    
    for (int i = 0; i < 5; i++) {
        raw_data[i] = dht22_read_byte();
    }
    
    // Tarkistussumma (Checksum)
    if (raw_data[4] != ((raw_data[0] + raw_data[1] + raw_data[2] + raw_data[3]) & 0xFF)) {
        return ESP_ERR_INVALID_CRC;
    }
    
    // Datan muunnos luvuiksi
    int16_t raw_humidity = (raw_data[0] << 8) | raw_data[1];
    int16_t raw_temperature = ((raw_data[2] & 0x7F) << 8) | raw_data[3];
    
    if (raw_data[2] & 0x80) { // Negatiivinen lämpötila
        raw_temperature = -raw_temperature;
    }
    
    data->humidity = (float)raw_humidity / 10.0;
    data->temperature = (float)raw_temperature / 10.0;
    
    return ESP_OK;
}
