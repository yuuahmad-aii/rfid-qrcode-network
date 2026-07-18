#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>

/* WIFI AND API CONFIGURATION - CHANGE THESE! */
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"
#define API_TEST_URL "http://192.168.137.1:3000/api/test"

#define BOOT_BUTTON_GPIO GPIO_NUM_9
#define BLINK_GPIO GPIO_NUM_8

#define UART_PORT_NUM      UART_NUM_1
#define UART_BAUD_RATE     115200
#define UART_TXD_PIN       GPIO_NUM_4
#define UART_RXD_PIN       GPIO_NUM_5
#define BUF_SIZE           1024

static const char *TAG = "app_main";

static led_strip_handle_t led_strip;
static SemaphoreHandle_t button_sem;
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static void set_led_color(uint8_t r, uint8_t g, uint8_t b) {
  if (led_strip) {
    led_strip_set_pixel(led_strip, 0, r, g, b);
    led_strip_refresh(led_strip);
  }
}

static void configure_led(void) {
  led_strip_config_t strip_config = {
      .strip_gpio_num = BLINK_GPIO,
      .max_leds = 1,
  };
  led_strip_spi_config_t spi_config = {
      .spi_bus = SPI2_HOST,
      .flags.with_dma = true,
  };
  ESP_ERROR_CHECK(
      led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
  led_strip_clear(led_strip);

  // Default state: Red (Not connected)
  set_led_color(50, 0, 0);
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    set_led_color(50, 0, 0); // Red
    esp_wifi_connect();
    ESP_LOGI(TAG, "retry to connect to the AP");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    set_led_color(0, 0, 50); // Blue (Connected, idle)
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

void wifi_init_sta(void) {
  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = WIFI_SSID,
              .password = WIFI_PASS,
              .threshold.authmode = WIFI_AUTH_WPA2_PSK,
          },
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_sta finished.");
}

static void IRAM_ATTR button_isr_handler(void *arg) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(button_sem, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt) { return ESP_OK; }

static void send_api_request(const char* rfid_uid) {
    // Check if wifi is connected
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, 0);
    if ((bits & WIFI_CONNECTED_BIT) == 0) {
        ESP_LOGW(TAG, "WiFi not connected, ignoring HTTP request.");
        return;
    }

    ESP_LOGI(TAG, "Sending HTTP request for RFID: %s", rfid_uid ? rfid_uid : "TEST");
    set_led_color(50, 50, 0); // Yellow (Sending)

    char url[256];
    if (rfid_uid != NULL) {
        snprintf(url, sizeof(url), "%s?rfid=%s", API_TEST_URL, rfid_uid);
    } else {
        snprintf(url, sizeof(url), "%s", API_TEST_URL);
    }

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Status = %d", status_code);

        if (status_code >= 200 && status_code < 300) {
            set_led_color(0, 50, 0); // Green (Success)
        } else {
            set_led_color(50, 0, 0); // Red (Failed)
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        set_led_color(50, 0, 0); // Red (Failed)
    }
    esp_http_client_cleanup(client);

    // Wait 1 second before reverting to idle state
    vTaskDelay(pdMS_TO_TICKS(1000));
    set_led_color(0, 0, 50); // Blue (Idle)
}

static void button_task(void *pvParameters) {
  while (1) {
    // Wait for button press (debouncing simple check)
    if (xSemaphoreTake(button_sem, portMAX_DELAY) == pdTRUE) {
      // Simple debounce: delay 50ms and check if button is still pressed
      vTaskDelay(pdMS_TO_TICKS(50));
      if (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {
        xSemaphoreTake(button_sem, 0);
        send_api_request(NULL);
      }
    }
  }
}

static void uart_task(void *pvParameters) {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TXD_PIN, UART_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    char rfid_str[32];
    int rfid_idx = 0;

    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            data[len] = '\0';
            ESP_LOGI(TAG, "UART received: %s", (char*)data);

            for (int i = 0; i < len; i++) {
                if (data[i] == '\n' || data[i] == '\r') {
                    if (rfid_idx > 0) {
                        rfid_str[rfid_idx] = '\0';
                        if (strncmp(rfid_str, "RFID:", 5) == 0) {
                            send_api_request(rfid_str + 5);
                        }
                        rfid_idx = 0;
                    }
                } else {
                    if (rfid_idx < sizeof(rfid_str) - 1) {
                        rfid_str[rfid_idx++] = data[i];
                    }
                }
            }
        }
    }
}

void app_main(void) {
  // Initialize NVS (required for WiFi)
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Initialize LED
  configure_led();

  // Initialize Button Semaphore
  button_sem = xSemaphoreCreateBinary();

  // Initialize Button GPIO
  gpio_config_t io_conf = {
      .intr_type = GPIO_INTR_NEGEDGE, // trigger on falling edge (press)
      .pin_bit_mask = (1ULL << BOOT_BUTTON_GPIO),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = 1,
  };
  gpio_config(&io_conf);

  // Install GPIO ISR service
  gpio_install_isr_service(0);
  // Hook ISR handler
  gpio_isr_handler_add(BOOT_BUTTON_GPIO, button_isr_handler,
                       (void *)BOOT_BUTTON_GPIO);

  // Create Tasks
  xTaskCreate(button_task, "button_task", 4096, NULL, 5, NULL);
  xTaskCreate(uart_task, "uart_task", 4096, NULL, 5, NULL);

  // Initialize WiFi
  wifi_init_sta();
}
