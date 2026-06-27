idf_component_register(SRCS "main.c"
                        "sensor/dht22.c"
                        "network/wifi.c"
                        "mqtt/mqtt_app.c"
                    INCLUDE_DIRS "."
                                "config"
                                "sensor"
                                "network"
                                "mqtt")
