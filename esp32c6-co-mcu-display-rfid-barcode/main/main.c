#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_crt_bundle.h"
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
#include <time.h>
#include "esp_netif_sntp.h"
#include "cJSON.h"

/* WIFI AND API CONFIGURATION - CHANGE THESE! */
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"
#define API_TEST_URL "http://192.168.137.1:3000/api/test"
#define API_TIME_URL "https://device.dsalute.id/api/v1/time.php"
#define API_TRANSACTIONS_URL "https://device.dsalute.id/api/v1/transactions.php"
#define API_KEY "dev_8dfa3625a4593e78de6c02e7b03b1daf7f3b13ba9cae2d81"

#define BOOT_BUTTON_GPIO GPIO_NUM_9
#define BLINK_GPIO GPIO_NUM_8

static const char *cloudflare_ca_pem = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDejCCAmKgAwIBAgIQf+UwvzMTQ77dghYQST2KGzANBgkqhkiG9w0BAQsFADBX\n" \
"MQswCQYDVQQGEwJCRTEZMBcGA1UEChMQR2xvYmFsU2lnbiBudi1zYTEQMA4GA1UE\n" \
"CxMHUm9vdCBDQTEbMBkGA1UEAxMSR2xvYmFsU2lnbiBSb290IENBMB4XDTIzMTEx\n" \
"NTAzNDMyMVoXDTI4MDEyODAwMDA0MlowRzELMAkGA1UEBhMCVVMxIjAgBgNVBAoT\n" \
"GUdvb2dsZSBUcnVzdCBTZXJ2aWNlcyBMTEMxFDASBgNVBAMTC0dUUyBSb290IFI0\n" \
"MHYwEAYHKoZIzj0CAQYFK4EEACIDYgAE83Rzp2iLYK5DuDXFgTB7S0md+8Fhzube\n" \
"Rr1r1WEYNa5A3XP3iZEwWus87oV8okB2O6nGuEfYKueSkWpz6bFyOZ8pn6KY019e\n" \
"WIZlD6GEZQbR3IvJx3PIjGov5cSr0R2Ko4H/MIH8MA4GA1UdDwEB/wQEAwIBhjAd\n" \
"BgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwDwYDVR0TAQH/BAUwAwEB/zAd\n" \
"BgNVHQ4EFgQUgEzW63T/STaj1dj8tT7FavCUHYwwHwYDVR0jBBgwFoAUYHtmGkUN\n" \
"l8qJUC99BM00qP/8/UswNgYIKwYBBQUHAQEEKjAoMCYGCCsGAQUFBzAChhpodHRw\n" \
"Oi8vaS5wa2kuZ29vZy9nc3IxLmNydDAtBgNVHR8EJjAkMCKgIKAehhxodHRwOi8v\n" \
"Yy5wa2kuZ29vZy9yL2dzcjEuY3JsMBMGA1UdIAQMMAowCAYGZ4EMAQIBMA0GCSqG\n" \
"SIb3DQEBCwUAA4IBAQAYQrsPBtYDh5bjP2OBDwmkoWhIDDkic574y04tfzHpn+cJ\n" \
"odI2D4SseesQ6bDrarZ7C30ddLibZatoKiws3UL9xnELz4ct92vID24FfVbiI1hY\n" \
"+SW6FoVHkNeWIP0GCbaM4C6uVdF5dTUsMVs/ZbzNnIdCp5Gxmx5ejvEau8otR/Cs\n" \
"kGN+hr/W5GvT1tMBjgWKZ1i4//emhA1JG1BbPzoLJQvyEotc03lXjTaCzv8mEbep\n" \
"8RqZ7a2CPsgRbuvTPBwcOMBBmuFeU88+FSBX6+7iP0il8b4Z0QFqIwwMHfs/L6K1\n" \
"vepuoxtGzi4CZ68zJpiq1UvSqTbFJjtbD4seiMHl\n" \
"-----END CERTIFICATE-----\n";

#define UART_PORT_NUM UART_NUM_1
#define UART_BAUD_RATE 115200
#define UART_TXD_PIN GPIO_NUM_4
#define UART_RXD_PIN GPIO_NUM_5
#define BUF_SIZE 1024

static const char *TAG = "app_main";

static led_strip_handle_t led_strip;
static SemaphoreHandle_t button_sem;
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
static SemaphoreHandle_t sync_time_sem;

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

static char api_response_buffer[2048];
static int api_response_len = 0;

esp_err_t _http_api_event_handler(esp_http_client_event_t *evt) {
  switch (evt->event_id) {
  case HTTP_EVENT_ON_DATA:
    if (api_response_len + evt->data_len < sizeof(api_response_buffer) - 1) {
      memcpy(api_response_buffer + api_response_len, evt->data,
             evt->data_len);
      api_response_len += evt->data_len;
      api_response_buffer[api_response_len] = '\0';
    }
    break;
  default:
    break;
  }
  return ESP_OK;
}

static void send_api_request(const char *identifier, const char *type) {
  // Check if wifi is connected
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT,
                                         pdFALSE, pdFALSE, 0);
  if ((bits & WIFI_CONNECTED_BIT) == 0) {
    ESP_LOGW(TAG, "WiFi not connected, ignoring HTTP request.");
    uart_write_bytes(UART_PORT_NUM, "DENIED:WiFi Disconnected\n", 25);
    return;
  }

  ESP_LOGI(TAG, "Sending HTTP request for %s: %s", type, identifier ? identifier : "TEST");
  set_led_color(50, 50, 0); // Yellow (Sending)

  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char timestamp[64];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S+07:00", &timeinfo);
  
  char uid[64];
  snprintf(uid, sizeof(uid), "DEV-001-%04d%02d%02d-%02d%02d%02d", 
           timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
           timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "transaction_uid", uid);
  cJSON_AddStringToObject(root, "timestamp", timestamp);
  cJSON_AddStringToObject(root, "identifier", identifier ? identifier : "TEST");
  cJSON_AddStringToObject(root, "identifier_type", type);
  cJSON_AddStringToObject(root, "time_source", "ntp");

  char *post_data = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);

  api_response_len = 0;
  memset(api_response_buffer, 0, sizeof(api_response_buffer));

  esp_http_client_config_t config = {
      .url = API_TRANSACTIONS_URL,
      .event_handler = _http_api_event_handler,
      .cert_pem = cloudflare_ca_pem,
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);
  esp_http_client_set_method(client, HTTP_METHOD_POST);
  esp_http_client_set_header(client, "X-API-Key", API_KEY);
  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_post_field(client, post_data, strlen(post_data));

  esp_err_t err = esp_http_client_perform(client);

  if (err == ESP_OK) {
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP Status = %d", status_code);

    cJSON *response_json = cJSON_Parse(api_response_buffer);
    if (response_json != NULL) {
      cJSON *success = cJSON_GetObjectItem(response_json, "success");
      if (cJSON_IsTrue(success)) {
        set_led_color(0, 50, 0); // Green (Success)
        cJSON *result = cJSON_GetObjectItem(response_json, "attendance_result");
        if (result && cJSON_IsObject(result)) {
           cJSON *name = cJSON_GetObjectItem(result, "name");
           char msg[64];
           snprintf(msg, sizeof(msg), "GRANTED:%s\n", name ? name->valuestring : "UNKNOWN");
           uart_write_bytes(UART_PORT_NUM, msg, strlen(msg));
        } else {
           uart_write_bytes(UART_PORT_NUM, "GRANTED:Berhasil\n", 17);
        }
      } else {
        set_led_color(50, 0, 0); // Red (Failed)
        cJSON *rejected = cJSON_GetObjectItem(response_json, "rejected");
        if (rejected && cJSON_IsArray(rejected)) {
           cJSON *item = cJSON_GetArrayItem(rejected, 0);
           if (item) {
             cJSON *reason = cJSON_GetObjectItem(item, "reason");
             char msg[64];
             snprintf(msg, sizeof(msg), "DENIED:%s\n", reason ? reason->valuestring : "Unknown error");
             uart_write_bytes(UART_PORT_NUM, msg, strlen(msg));
           } else {
             uart_write_bytes(UART_PORT_NUM, "DENIED:Ditolak Server\n", 22);
           }
        } else {
           uart_write_bytes(UART_PORT_NUM, "DENIED:Ditolak Server\n", 22);
        }
      }
      cJSON_Delete(response_json);
    } else {
      uart_write_bytes(UART_PORT_NUM, "DENIED:Parse Error\n", 19);
      set_led_color(50, 0, 0); // Red
    }
  } else {
    ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    set_led_color(50, 0, 0); // Red (Failed)
    uart_write_bytes(UART_PORT_NUM, "DENIED:Koneksi Gagal\n", 21);
  }
  esp_http_client_cleanup(client);
  free(post_data);

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
        send_api_request("TEST", "rfid");
      }
    }
  }
}

static char time_response_buffer[512];
static int time_response_len = 0;

esp_err_t _http_time_event_handler(esp_http_client_event_t *evt) {
  switch (evt->event_id) {
  case HTTP_EVENT_ON_DATA:
    if (time_response_len + evt->data_len < sizeof(time_response_buffer) - 1) {
      memcpy(time_response_buffer + time_response_len, evt->data,
             evt->data_len);
      time_response_len += evt->data_len;
      time_response_buffer[time_response_len] = '\0';
    }
    break;
  default:
    break;
  }
  return ESP_OK;
}

static void sync_time_task(void *pvParameters) {
  while (1) {
    EventBits_t bits =
        xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE,
                            pdFALSE, portMAX_DELAY);
    if ((bits & WIFI_CONNECTED_BIT) != 0) {
      // Ensure system time is set for TLS certificate validation
      time_t now;
      struct tm timeinfo;
      time(&now);
      localtime_r(&now, &timeinfo);
      if (timeinfo.tm_year < (2020 - 1900)) {
        ESP_LOGI(TAG, "System time is not set. Syncing via SNTP...");
        esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
        esp_netif_sntp_init(&sntp_config);

        ESP_LOGI(TAG, "Waiting for system time to be set...");
        esp_err_t wait_err = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000));
        time(&now);
        localtime_r(&now, &timeinfo);
        if (timeinfo.tm_year >= (2020 - 1900)) {
          ESP_LOGI(TAG, "System time successfully set to year %d", timeinfo.tm_year + 1900);
          setenv("TZ", "WIB-7", 1);
          tzset();
        } else {
          ESP_LOGE(TAG, "Failed to set system time via SNTP!");
        }
        esp_netif_sntp_deinit();
      }

      ESP_LOGI(TAG, "Fetching time from server API...");
      time_response_len = 0;
      memset(time_response_buffer, 0, sizeof(time_response_buffer));

      esp_http_client_config_t config = {
          .url = API_TIME_URL,
          .event_handler = _http_time_event_handler,
          .cert_pem = cloudflare_ca_pem,
      };
      esp_http_client_handle_t client = esp_http_client_init(&config);
      esp_http_client_set_header(client, "X-API-Key", API_KEY);

      esp_err_t err = esp_http_client_perform(client);
      if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code >= 200 && status_code < 300) {
          char *time_start = strstr(time_response_buffer, "\"server_time\":\"");
          if (time_start) {
            time_start += 15;
            char time_str[32] = {0};
            strncpy(time_str, time_start, 19);
            time_str[10] = ' '; // Replace 'T' with space

            char uart_msg[64];
            snprintf(uart_msg, sizeof(uart_msg), "TIME:%s\n", time_str);
            uart_write_bytes(UART_PORT_NUM, uart_msg, strlen(uart_msg));
            ESP_LOGI(TAG, "Sent to STM32: %s", uart_msg);
          } else {
            ESP_LOGE(TAG, "Failed to parse time. Response: %s",
                     time_response_buffer);
            uart_write_bytes(UART_PORT_NUM, "TIME_ERR:PARSE_FAIL\n", 20);
          }
        } else {
          ESP_LOGE(TAG, "HTTP GET time failed status: %d", status_code);
          char err_msg[64];
          snprintf(err_msg, sizeof(err_msg), "TIME_ERR:HTTP_%d\n", status_code);
          uart_write_bytes(UART_PORT_NUM, err_msg, strlen(err_msg));
        }
      } else {
        ESP_LOGE(TAG, "HTTP GET time failed: %s", esp_err_to_name(err));
        uart_write_bytes(UART_PORT_NUM, "TIME_ERR:CONN_FAIL\n", 19);
      }
      esp_http_client_cleanup(client);

      // Wait up to 1 hour before next sync, or until triggered
      xSemaphoreTake(sync_time_sem, pdMS_TO_TICKS(3600000));
    }
  }
}

static void uart_task(void *pvParameters) {
  uart_config_t uart_config = {
      .baud_rate = UART_BAUD_RATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };

  ESP_ERROR_CHECK(
      uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TXD_PIN, UART_RXD_PIN,
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

  uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
  char rfid_str[32];
  int rfid_idx = 0;

  while (1) {
    int len =
        uart_read_bytes(UART_PORT_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(100));
    if (len > 0) {
      data[len] = '\0';
      ESP_LOGI(TAG, "UART received: %s", (char *)data);

      for (int i = 0; i < len; i++) {
        if (data[i] == '\n' || data[i] == '\r') {
          if (rfid_idx > 0) {
            rfid_str[rfid_idx] = '\0';
            if (strncmp(rfid_str, "RFID:", 5) == 0) {
              send_api_request(rfid_str + 5, "rfid");
            } else if (strncmp(rfid_str, "BARCODE:", 8) == 0) {
              send_api_request(rfid_str + 8, "qr");
            } else if (strncmp(rfid_str, "CMD:SYNC_TIME", 13) == 0) {
              xSemaphoreGive(sync_time_sem);
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
  
  // Initialize Sync Time Semaphore
  sync_time_sem = xSemaphoreCreateBinary();

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

  // Initialize WiFi first so event group is created
  wifi_init_sta();

  // Create Tasks
  xTaskCreate(button_task, "button_task", 4096, NULL, 5, NULL);
  xTaskCreate(uart_task, "uart_task", 4096, NULL, 5, NULL);
  xTaskCreate(sync_time_task, "sync_time", 4096, NULL, 4, NULL);
}
