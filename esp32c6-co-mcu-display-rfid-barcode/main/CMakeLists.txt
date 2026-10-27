idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_wifi esp_event nvs_flash esp_netif esp_http_client esp_http_server app_update esp_partition led_strip esp_driver_gpio esp_driver_uart mbedtls esp-tls cjson esp_hw_support)
