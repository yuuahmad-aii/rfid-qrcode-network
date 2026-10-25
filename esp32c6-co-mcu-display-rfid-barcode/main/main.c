#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "esp_wifi_he.h"
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
#include "esp_task_wdt.h"
#include "cJSON.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"

/* WIFI AND API CONFIGURATION - CHANGE THESE! */
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"
#define API_TEST_URL "http://192.168.137.1:3000/api/test"
#define API_TIME_URL "https://device.dsalute.id/api/v1/time.php"
#define API_TRANSACTIONS_URL "https://device.dsalute.id/api/v1/transactions.php"
#define API_REGISTER_URL "https://device.dsalute.id/api/v1/register.php"
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


static const char *TAG = "app_main";

static void uart_send_and_log(const char* msg) {
    char clean_msg[128];
    strncpy(clean_msg, msg, sizeof(clean_msg)-1);
    clean_msg[sizeof(clean_msg)-1] = '\0';
    char *newline = strchr(clean_msg, '\n');
    if (newline) *newline = '\0';
    
    ESP_LOGI(TAG, "UART Sent: %s", clean_msg);
    uart_write_bytes(UART_PORT_NUM, msg, strlen(msg));
}
#define UART_BAUD_RATE 115200
#define UART_TXD_PIN GPIO_NUM_4
#define UART_RXD_PIN GPIO_NUM_5
#define BUF_SIZE 1024


static led_strip_handle_t led_strip;
static SemaphoreHandle_t button_sem;
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
static SemaphoreHandle_t sync_time_sem;
static cJSON *s_bulk_transactions_array = NULL;

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
  set_led_color(16, 0, 0);
}

static char s_device_code[32] = "GATE-001";
static char s_device_name[32] = "Gate Utama";
static char s_api_key[64] = API_KEY;
static bool s_wifi_connected_prev = false;
static int s_wifi_retry_count = 0;
static esp_netif_t *s_sta_netif = NULL;

static void send_wifi_status_info(void) {
  wifi_ap_record_t ap_info;
  esp_netif_ip_info_t ip_info;
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  char proto_str[32] = "Wi-Fi 4 (802.11n)";
  char ssid_str[34] = "-";
  char ip_str[16] = "0.0.0.0";
  int rssi = 0;
  bool is_connected = false;

  if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
    is_connected = true;
    strncpy(ssid_str, (char *)ap_info.ssid, sizeof(ssid_str) - 1);
    rssi = ap_info.rssi;
    if (ap_info.phy_11ax) {
      strncpy(proto_str, "Wi-Fi 6 (802.11ax)", sizeof(proto_str) - 1);
    } else if (ap_info.phy_11n) {
      strncpy(proto_str, "Wi-Fi 4 (802.11n)", sizeof(proto_str) - 1);
    } else if (ap_info.phy_11g) {
      strncpy(proto_str, "802.11g", sizeof(proto_str) - 1);
    } else if (ap_info.phy_11b) {
      strncpy(proto_str, "802.11b", sizeof(proto_str) - 1);
    }
  }

  if (s_sta_netif && esp_netif_get_ip_info(s_sta_netif, &ip_info) == ESP_OK) {
    esp_ip4addr_ntoa(&ip_info.ip, ip_str, sizeof(ip_str));
  }

  char mac_str[18];
  snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  char msg[160];
  snprintf(msg, sizeof(msg), "WIFI_INFO:%s|%s|%s|%d dBm|%s|%s\n",
           is_connected ? "CONNECTED" : "DISCONNECTED",
           ssid_str, ip_str, rssi, proto_str, mac_str);
  uart_send_and_log(msg);
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    s_wifi_retry_count = 0;
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    set_led_color(16, 0, 0); // Red
    s_wifi_retry_count++;
    if (s_wifi_connected_prev) {
      s_wifi_connected_prev = false;
      uart_send_and_log("WIFI:FAIL\n");
      vTaskDelay(pdMS_TO_TICKS(50));
      send_wifi_status_info();
    }
    ESP_LOGW(TAG, "Wi-Fi disconnected. Retry attempt #%d...", s_wifi_retry_count);
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_wifi_connect();
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_wifi_retry_count = 0;
    set_led_color(0, 0, 8); // Dim Blue (Connected, idle)

    /* 1. Aktifkan Maximum Modem-Sleep (DTIM Sleep) untuk menghemat baterai secara drastis */
    esp_wifi_set_ps(WIFI_PS_MAX_MODEM);

    /* 2. Negosiasikan Wi-Fi 6 (802.11ax) Individual Target Wake Time (iTWT) jika router mendukung */
    wifi_itwt_setup_config_t itwt_cfg = {
        .setup_cmd = TWT_REQUEST,
        .trigger = 1,
        .flow_type = 0,         /* Announced individual TWT */
        .wake_invl_expn = 10,   /* Target wake interval ~1024 ms */
        .wake_invl_mant = 1000,
        .min_wake_dura = 255,   /* Nominal minimum wake duration */
        .timeout_time_ms = 5000,
    };
    esp_err_t twt_err = esp_wifi_sta_itwt_setup(&itwt_cfg);
    if (twt_err == ESP_OK) {
        ESP_LOGI(TAG, "Wi-Fi 6 (802.11ax) Individual TWT negotiation initiated!");
    } else {
        ESP_LOGI(TAG, "Wi-Fi 6 TWT not applicable (%s). Standard DTIM Max Modem-Sleep active.", esp_err_to_name(twt_err));
    }

    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    s_wifi_connected_prev = true;
    uart_send_and_log("WIFI:OK\n");
    vTaskDelay(pdMS_TO_TICKS(50));
    send_wifi_status_info();
  }
}

void wifi_init_sta(void) {
  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  s_sta_netif = esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

  char ssid[34] = WIFI_SSID;
  char pass[66] = WIFI_PASS;
  char ip[16] = "";

  nvs_handle_t my_handle;
  if (nvs_open("storage", NVS_READONLY, &my_handle) == ESP_OK) {
      size_t len = sizeof(ssid);
      nvs_get_str(my_handle, "ssid", ssid, &len);
      len = sizeof(pass);
      nvs_get_str(my_handle, "pass", pass, &len);
      len = sizeof(ip);
      nvs_get_str(my_handle, "ip", ip, &len);
      len = sizeof(s_device_code);
      nvs_get_str(my_handle, "dev_code", s_device_code, &len);
      len = sizeof(s_device_name);
      nvs_get_str(my_handle, "dev_name", s_device_name, &len);
      len = sizeof(s_api_key);
      if (nvs_get_str(my_handle, "api_key", s_api_key, &len) == ESP_OK && strlen(s_api_key) > 0) {
          ESP_LOGI(TAG, "Loaded API Key from NVS: %s", s_api_key);
      }
      nvs_close(my_handle);
  }

  /* Jika dev_code masih default GATE-001 atau kosong, gunakan default unik berdasarkan MAC address Wi-Fi */
  if (strlen(s_device_code) == 0 || strcmp(s_device_code, "GATE-001") == 0) {
      uint8_t mac[6] = {0};
      if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
          snprintf(s_device_code, sizeof(s_device_code), "GATE-%02X%02X", mac[4], mac[5]);
          if (strlen(s_device_name) == 0 || strcmp(s_device_name, "Gate Utama") == 0) {
              snprintf(s_device_name, sizeof(s_device_name), "Gate %02X%02X", mac[4], mac[5]);
          }
      }
  }

  /* Hanya terapkan Static IP jika IP valid dan bukan DHCP / 0.0.0.0 */
  if (strlen(ip) > 0 && strcmp(ip, "0.0.0.0") != 0 && strcmp(ip, "DHCP") != 0) {
      esp_netif_ip_info_t ip_info;
      memset(&ip_info, 0, sizeof(ip_info));
      if (ip4addr_aton(ip, (ip4_addr_t *)&ip_info.ip) && ip_info.ip.addr != 0) {
          esp_netif_dhcpc_stop(s_sta_netif);
          char gw_str[16];
          strncpy(gw_str, ip, sizeof(gw_str));
          char *last_dot = strrchr(gw_str, '.');
          if (last_dot) {
              strcpy(last_dot + 1, "1");
          }
          ip4addr_aton(gw_str, (ip4_addr_t *)&ip_info.gw);
          ip4addr_aton("255.255.255.0", (ip4_addr_t *)&ip_info.netmask);
          esp_netif_set_ip_info(s_sta_netif, &ip_info);
          ESP_LOGI(TAG, "Static IP configured: %s, GW: %s", ip, gw_str);
      }
  } else {
      esp_netif_dhcpc_start(s_sta_netif);
      ESP_LOGI(TAG, "Using automatic DHCP for IP assignment");
  }

  wifi_config_t wifi_config = {
      .sta = {
              .threshold.authmode = (strlen(pass) == 0) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA_PSK,
              .listen_interval = 3, /* Dengarkan DTIM beacon router setiap 3 interval untuk menghemat daya */
              .pmf_cfg = {
                  .capable = true,
                  .required = false,
              },
      },
  };
  strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
  strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  /* Jangan aktifkan power saving modem sleep sebelum terhubung ke AP agar negosiasi DHCP lancar */
  esp_wifi_set_ps(WIFI_PS_NONE);

  ESP_LOGI(TAG, "wifi_init_sta started for SSID: '%s'", ssid);
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
    uart_send_and_log("DENIED:WiFi Disconnected\n");
    return;
  }

  ESP_LOGI(TAG, "Sending HTTP request for %s: %s", type, identifier ? identifier : "TEST");
  set_led_color(20, 20, 0); // Dim Yellow (Sending)

  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char timestamp[64];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S+07:00", &timeinfo);
  
  char uid[64];
  snprintf(uid, sizeof(uid), "%s-%04d%02d%02d-%02d%02d%02d", 
           s_device_code,
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

  esp_task_wdt_reset();
  esp_err_t err = esp_http_client_perform(client);
  esp_task_wdt_reset();

  if (err == ESP_OK) {
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP Status = %d", status_code);

    cJSON *response_json = cJSON_Parse(api_response_buffer);
    if (response_json != NULL) {
      cJSON *success = cJSON_GetObjectItem(response_json, "success");
      cJSON *result = cJSON_GetObjectItem(response_json, "attendance_result");
      
      if ((success && cJSON_IsTrue(success)) || result != NULL) {
        set_led_color(0, 20, 0); // Dim Green (Success)
        if (result && cJSON_IsObject(result)) {
           cJSON *id_json = cJSON_GetObjectItem(result, "id");
           cJSON *name_json = cJSON_GetObjectItem(result, "name");
           if (!name_json) name_json = cJSON_GetObjectItem(result, "nama");
           cJSON *pos_json = cJSON_GetObjectItem(result, "position");
           if (!pos_json) pos_json = cJSON_GetObjectItem(result, "posisi");
           if (!pos_json) pos_json = cJSON_GetObjectItem(result, "jabatan");

           char id_str[32] = "-";
           if (id_json) {
               if (cJSON_IsString(id_json)) snprintf(id_str, sizeof(id_str), "%s", id_json->valuestring);
               else if (cJSON_IsNumber(id_json)) snprintf(id_str, sizeof(id_str), "%d", id_json->valueint);
           }

           const char *name_str = (name_json && cJSON_IsString(name_json)) ? name_json->valuestring : "-";
           const char *pos_str = (pos_json && cJSON_IsString(pos_json)) ? pos_json->valuestring : "-";

           /* VALIDASI KETAT: Hanya kirim GRANTED jika kartu memiliki data Karyawan sah */
           if (strcmp(name_str, "-") != 0 && strlen(name_str) > 0 &&
               strcasecmp(name_str, "Akses Diterima") != 0 &&
               strcasecmp(name_str, "UNKNOWN") != 0) {
             set_led_color(0, 20, 0); // Dim Green (Success)
             char msg[128];
             snprintf(msg, sizeof(msg), "GRANTED:%s|%s|%s\n",
                      (identifier && strlen(identifier) > 0) ? identifier : id_str,
                      name_str, pos_str);
             uart_send_and_log(msg);
           } else {
             /* Kartu tercatat di server tapi belum di-mapping ke Karyawan */
             set_led_color(20, 0, 0); // Dim Red (Failed)
             uart_send_and_log("DENIED:Belum Terdaftar\n");
           }
        } else {
           /* Kartu tidak memiliki objek data karyawan */
           set_led_color(20, 0, 0); // Dim Red (Failed)
           uart_send_and_log("DENIED:Belum Terdaftar\n");
        }
      } else {
        set_led_color(20, 0, 0); // Dim Red (Failed)
        cJSON *rejected = cJSON_GetObjectItem(response_json, "rejected");
        char rej_reason[64] = "Ditolak Server";
        if (rejected && cJSON_IsArray(rejected)) {
           cJSON *item = cJSON_GetArrayItem(rejected, 0);
           if (item) {
             cJSON *reason = cJSON_GetObjectItem(item, "reason");
             if (reason && reason->valuestring) {
               strncpy(rej_reason, reason->valuestring, sizeof(rej_reason) - 1);
             }
           }
        } else {
           cJSON *msg_obj = cJSON_GetObjectItem(response_json, "message");
           if (msg_obj && msg_obj->valuestring) {
             strncpy(rej_reason, msg_obj->valuestring, sizeof(rej_reason) - 1);
           }
        }
        char msg[96];
        snprintf(msg, sizeof(msg), "DENIED:%s\n", rej_reason);
        uart_send_and_log(msg);
      }
      cJSON_Delete(response_json);
    } else {
      uart_send_and_log("DENIED:Parse Error\n");
      set_led_color(20, 0, 0); // Dim Red
    }
  } else {
    ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    set_led_color(20, 0, 0); // Dim Red (Failed)
    uart_send_and_log("DENIED:Koneksi Gagal\n");
  }
  esp_http_client_cleanup(client);
  free(post_data);

  // Wait 1 second before reverting to idle state
  vTaskDelay(pdMS_TO_TICKS(1000));
  set_led_color(0, 0, 8); // Dim Blue (Idle low power)
}

static void send_custom_api_request(const char *trans_uid, const char *timestamp, const char *identifier, const char *type, const char *time_source) {
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT,
                                         pdFALSE, pdFALSE, 0);
  if ((bits & WIFI_CONNECTED_BIT) == 0) {
    ESP_LOGW(TAG, "WiFi not connected, ignoring single transaction HTTP request.");
    uart_send_and_log("DENIED:WiFi Disconnected\n");
    return;
  }

  ESP_LOGI(TAG, "Sending custom transaction: UID=%s, ID=%s", trans_uid, identifier);
  set_led_color(20, 20, 0); // Yellow

  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "transaction_uid", trans_uid);
  cJSON_AddStringToObject(root, "timestamp", timestamp);
  cJSON_AddStringToObject(root, "identifier", identifier ? identifier : "TEST");
  cJSON_AddStringToObject(root, "identifier_type", type ? type : "rfid");
  cJSON_AddStringToObject(root, "time_source", time_source ? time_source : "rtc");

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
  esp_http_client_set_header(client, "X-API-Key", s_api_key);
  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_post_field(client, post_data, strlen(post_data));

  esp_task_wdt_reset();
  esp_err_t err = esp_http_client_perform(client);
  esp_task_wdt_reset();

  if (err == ESP_OK) {
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "Custom HTTP Status = %d", status_code);

    cJSON *response_json = cJSON_Parse(api_response_buffer);
    if (response_json != NULL) {
      cJSON *success = cJSON_GetObjectItem(response_json, "success");
      cJSON *result = cJSON_GetObjectItem(response_json, "attendance_result");

      if ((success && cJSON_IsTrue(success)) || result != NULL) {
        if (result && cJSON_IsObject(result)) {
           cJSON *id_json = cJSON_GetObjectItem(result, "id");
           cJSON *name_json = cJSON_GetObjectItem(result, "name");
           if (!name_json) name_json = cJSON_GetObjectItem(result, "nama");
           cJSON *pos_json = cJSON_GetObjectItem(result, "position");
           if (!pos_json) pos_json = cJSON_GetObjectItem(result, "posisi");
           if (!pos_json) pos_json = cJSON_GetObjectItem(result, "jabatan");

           char id_str[32] = "-";
           if (id_json) {
               if (cJSON_IsString(id_json)) snprintf(id_str, sizeof(id_str), "%s", id_json->valuestring);
               else if (cJSON_IsNumber(id_json)) snprintf(id_str, sizeof(id_str), "%d", id_json->valueint);
           }
           const char *name_str = (name_json && cJSON_IsString(name_json)) ? name_json->valuestring : "-";
           const char *pos_str = (pos_json && cJSON_IsString(pos_json)) ? pos_json->valuestring : "-";

           if (strcmp(name_str, "-") != 0 && strlen(name_str) > 0 &&
               strcasecmp(name_str, "Akses Diterima") != 0 &&
               strcasecmp(name_str, "UNKNOWN") != 0) {
             set_led_color(0, 20, 0); // Green
             char ok_msg[96];
             snprintf(ok_msg, sizeof(ok_msg), "TXN_OK:%s\n", trans_uid);
             uart_send_and_log(ok_msg);
             vTaskDelay(pdMS_TO_TICKS(60));

             char msg[128];
             snprintf(msg, sizeof(msg), "GRANTED:%s|%s|%s\n",
                      (identifier && strlen(identifier) > 0) ? identifier : id_str,
                      name_str, pos_str);
             uart_send_and_log(msg);
           } else {
             /* Kartu belum di-mapping ke karyawan */
             set_led_color(20, 0, 0); // Red
             char err_msg[96];
             snprintf(err_msg, sizeof(err_msg), "TXN_ERR:%s|Belum Terdaftar\n", trans_uid);
             uart_send_and_log(err_msg);
           }
        } else {
           /* Tidak ada objek attendance_result */
           set_led_color(20, 0, 0); // Red
           char err_msg[96];
           snprintf(err_msg, sizeof(err_msg), "TXN_ERR:%s|Belum Terdaftar\n", trans_uid);
           uart_send_and_log(err_msg);
        }
      } else {
        set_led_color(20, 0, 0); // Red
        cJSON *rejected = cJSON_GetObjectItem(response_json, "rejected");
        char rej_reason[64] = "Ditolak Server";
        if (rejected && cJSON_IsArray(rejected)) {
           cJSON *item = cJSON_GetArrayItem(rejected, 0);
           if (item) {
             cJSON *reason = cJSON_GetObjectItem(item, "reason");
             if (reason && reason->valuestring) {
               strncpy(rej_reason, reason->valuestring, sizeof(rej_reason) - 1);
             }
           }
        } else {
           cJSON *msg_obj = cJSON_GetObjectItem(response_json, "message");
           if (msg_obj && msg_obj->valuestring) {
             strncpy(rej_reason, msg_obj->valuestring, sizeof(rej_reason) - 1);
           }
        }
        char err_msg[96];
        snprintf(err_msg, sizeof(err_msg), "TXN_ERR:%s|%s\n", trans_uid, rej_reason);
        uart_send_and_log(err_msg);
      }
      cJSON_Delete(response_json);
    } else {
      set_led_color(20, 0, 0);
      uart_send_and_log("DENIED:Parse Error\n");
    }
  } else {
    ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    set_led_color(20, 0, 0);
    char err_msg[96];
    snprintf(err_msg, sizeof(err_msg), "TXN_ERR:%s|Koneksi Gagal\n", trans_uid);
    uart_send_and_log(err_msg);
  }
  esp_http_client_cleanup(client);
  free(post_data);

  vTaskDelay(pdMS_TO_TICKS(500));
  set_led_color(0, 0, 8); // Idle Blue
}

static void send_bulk_api_request(cJSON *arr) {
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT,
                                         pdFALSE, pdFALSE, 0);
  if ((bits & WIFI_CONNECTED_BIT) == 0) {
    ESP_LOGW(TAG, "WiFi not connected, ignoring bulk HTTP request.");
    cJSON_Delete(arr);
    uart_send_and_log("BULK_ERR:Wi-Fi Tidak Terhubung!\n");
    return;
  }

  int count = cJSON_GetArraySize(arr);
  ESP_LOGI(TAG, "Sending Bulk request with %d items...", count);
  set_led_color(20, 20, 0); // Yellow

  cJSON *root = cJSON_CreateObject();
  cJSON_AddItemToObject(root, "transactions", arr);
  char *post_data = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);

  api_response_len = 0;
  memset(api_response_buffer, 0, sizeof(api_response_buffer));

  esp_http_client_config_t config = {
      .url = API_TRANSACTIONS_URL,
      .event_handler = _http_api_event_handler,
      .cert_pem = cloudflare_ca_pem,
      .timeout_ms = 10000,
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);
  esp_http_client_set_method(client, HTTP_METHOD_POST);
  esp_http_client_set_header(client, "X-API-Key", s_api_key);
  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_post_field(client, post_data, strlen(post_data));

  esp_task_wdt_reset();
  esp_err_t err = esp_http_client_perform(client);
  esp_task_wdt_reset();

  int accepted_count = 0;
  int rejected_count = 0;

  if (err == ESP_OK) {
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "Bulk HTTP Status = %d, Resp: %s", status_code, api_response_buffer);

    if (status_code == 401) {
      set_led_color(20, 0, 0); // Red
      uart_send_and_log("BULK_ERR:API Key Belum Diaktifkan di Dashboard\n");
      esp_http_client_cleanup(client);
      free(post_data);
      vTaskDelay(pdMS_TO_TICKS(500));
      set_led_color(0, 0, 8);
      return;
    }

    cJSON *response_json = cJSON_Parse(api_response_buffer);
    if (response_json != NULL) {
      cJSON *succ_item = cJSON_GetObjectItem(response_json, "success");
      cJSON *acc_item = cJSON_GetObjectItem(response_json, "accepted_count");
      cJSON *rej_item = cJSON_GetObjectItem(response_json, "rejected_count");
      cJSON *acc_arr = cJSON_GetObjectItem(response_json, "accepted");
      cJSON *rej_arr = cJSON_GetObjectItem(response_json, "rejected");

      if (acc_item && cJSON_IsNumber(acc_item)) {
        accepted_count = acc_item->valueint;
      } else if (acc_item && cJSON_IsString(acc_item)) {
        accepted_count = atoi(acc_item->valuestring);
      } else if (acc_arr && cJSON_IsArray(acc_arr)) {
        accepted_count = cJSON_GetArraySize(acc_arr);
      }

      if (rej_item && cJSON_IsNumber(rej_item)) {
        rejected_count = rej_item->valueint;
      } else if (rej_item && cJSON_IsString(rej_item)) {
        rejected_count = atoi(rej_item->valuestring);
      } else if (rej_arr && cJSON_IsArray(rej_arr)) {
        rejected_count = cJSON_GetArraySize(rej_arr);
      }

      /* Fallback jika server mengembalikan HTTP 200 atau success:true tanpa counter eksplisit */
      if (accepted_count == 0 && rejected_count == 0) {
        if ((succ_item && cJSON_IsTrue(succ_item)) || (status_code >= 200 && status_code < 300)) {
          accepted_count = count;
        }
      }
      cJSON_Delete(response_json);
    } else {
      if (status_code >= 200 && status_code < 300) {
        accepted_count = count;
      }
    }
    set_led_color(0, 20, 0); // Green
  } else {
    ESP_LOGE(TAG, "Bulk HTTP failed: %s", esp_err_to_name(err));
    set_led_color(20, 0, 0); // Red
    char err_buf[64];
    snprintf(err_buf, sizeof(err_buf), "BULK_ERR:Gagal Hubungi Server (%s)\n", esp_err_to_name(err));
    uart_send_and_log(err_buf);
    esp_http_client_cleanup(client);
    free(post_data);
    vTaskDelay(pdMS_TO_TICKS(500));
    set_led_color(0, 0, 8);
    return;
  }

  esp_http_client_cleanup(client);
  free(post_data);

  char ack_msg[64];
  snprintf(ack_msg, sizeof(ack_msg), "BULK_ACK:%d|%d\n", accepted_count, rejected_count);
  uart_send_and_log(ack_msg);

  vTaskDelay(pdMS_TO_TICKS(500));
  set_led_color(0, 0, 8); // Idle Blue
}

static void send_register_device_request(const char *code, const char *name) {
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT,
                                         pdFALSE, pdFALSE, 0);
  if ((bits & WIFI_CONNECTED_BIT) == 0) {
    ESP_LOGW(TAG, "WiFi not connected, cannot register device.");
    uart_send_and_log("REG_ERR:Wi-Fi Tidak Terhubung!\n");
    return;
  }

  set_led_color(20, 20, 0); // Yellow

  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "device_code", code);
  cJSON_AddStringToObject(root, "device_name", name);
  cJSON_AddStringToObject(root, "location", "Lobby");
  cJSON_AddStringToObject(root, "firmware_version", "1.0.0");
  char *post_data = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);

  api_response_len = 0;
  memset(api_response_buffer, 0, sizeof(api_response_buffer));

  esp_http_client_config_t config = {
      .url = API_REGISTER_URL,
      .event_handler = _http_api_event_handler,
      .cert_pem = cloudflare_ca_pem,
      .timeout_ms = 8000,
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);
  esp_http_client_set_method(client, HTTP_METHOD_POST);
  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_post_field(client, post_data, strlen(post_data));

  esp_task_wdt_reset();
  esp_err_t err = esp_http_client_perform(client);
  esp_task_wdt_reset();

  if (err == ESP_OK) {
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "Register HTTP Status = %d, Resp: %s", status_code, api_response_buffer);
    cJSON *resp_json = cJSON_Parse(api_response_buffer);
    if (resp_json != NULL) {
      cJSON *succ = cJSON_GetObjectItem(resp_json, "success");
      cJSON *key = cJSON_GetObjectItem(resp_json, "api_key");
      if (succ && cJSON_IsTrue(succ) && key && cJSON_IsString(key)) {
        strncpy(s_api_key, key->valuestring, sizeof(s_api_key) - 1);
        s_api_key[sizeof(s_api_key) - 1] = '\0';

        nvs_handle_t my_handle;
        if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
          nvs_set_str(my_handle, "api_key", s_api_key);
          nvs_commit(my_handle);
          nvs_close(my_handle);
          ESP_LOGI(TAG, "Saved new API Key from register to NVS: %s", s_api_key);
        }
        set_led_color(0, 20, 0); // Green
        char msg[96];
        snprintf(msg, sizeof(msg), "REG_OK:Device %s Terdaftar! Aktifkan di Dashboard\n", code);
        uart_send_and_log(msg);
      } else {
        set_led_color(20, 0, 0); // Red
        cJSON *msg_item = cJSON_GetObjectItem(resp_json, "message");
        const char *err_str = (msg_item && cJSON_IsString(msg_item)) ? msg_item->valuestring : "Ditolak Server";
        char msg[96];
        snprintf(msg, sizeof(msg), "REG_ERR:%.80s\n", err_str);
        uart_send_and_log(msg);
      }
      cJSON_Delete(resp_json);
    } else {
      set_led_color(20, 0, 0);
      uart_send_and_log("REG_ERR:Format JSON Tidak Valid\n");
    }
  } else {
    ESP_LOGE(TAG, "Register HTTP failed: %s", esp_err_to_name(err));
    set_led_color(20, 0, 0);
    char msg[96];
    snprintf(msg, sizeof(msg), "REG_ERR:Gagal Hubungi Server (%s)\n", esp_err_to_name(err));
    uart_send_and_log(msg);
  }

  esp_http_client_cleanup(client);
  free(post_data);

  vTaskDelay(pdMS_TO_TICKS(500));
  set_led_color(0, 0, 8); // Idle Blue
}

static void button_task(void *pvParameters) {
  esp_task_wdt_add(NULL);
  while (1) {
    esp_task_wdt_reset();
    // Wait for button press (timeout 1000ms agar loop rutin menyegarkan watchdog)
    if (xSemaphoreTake(button_sem, pdMS_TO_TICKS(1000)) == pdTRUE) {
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
      bool synced = false;

      /* 1. Sinkronisasi via standard NTP (SNTP pool.ntp.org) */
      ESP_LOGI(TAG, "Syncing time via SNTP...");
      esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
      esp_netif_sntp_init(&sntp_config);

      if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(8000)) == ESP_OK) {
        setenv("TZ", "WIB-7", 1);
        tzset();
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        if (timeinfo.tm_year >= (2020 - 1900)) {
          char uart_msg[64];
          snprintf(uart_msg, sizeof(uart_msg), "TIME:%04d-%02d-%02d %02d:%02d:%02d\n",
                   timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                   timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
          uart_send_and_log(uart_msg);
          ESP_LOGI(TAG, "Sent SNTP time to STM32: %s", uart_msg);
          synced = true;
        }
      }
      esp_netif_sntp_deinit();

      /* 2. Jika SNTP gagal, coba server API */
      if (!synced && strlen(API_TIME_URL) > 0) {
        ESP_LOGI(TAG, "Fetching time from server API...");
        time_response_len = 0;
        memset(time_response_buffer, 0, sizeof(time_response_buffer));

        esp_http_client_config_t config = {
            .url = API_TIME_URL,
            .event_handler = _http_time_event_handler,
            .cert_pem = cloudflare_ca_pem,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_http_client_set_header(client, "X-API-Key", s_api_key);

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
              uart_send_and_log(uart_msg);
              ESP_LOGI(TAG, "Sent API time to STM32: %s", uart_msg);
              synced = true;
            } else {
              uart_send_and_log("TIME_ERR:PARSE_FAIL\n");
            }
          } else {
            char err_msg[64];
            snprintf(err_msg, sizeof(err_msg), "TIME_ERR:HTTP_%d\n", status_code);
            uart_send_and_log(err_msg);
          }
        } else {
          uart_send_and_log("TIME_ERR:CONN_FAIL\n");
        }
        esp_http_client_cleanup(client);
      }

      if (!synced) {
        ESP_LOGE(TAG, "All time sync methods failed!");
        uart_send_and_log("TIME_ERR:NTP_TIMEOUT\n");
      }

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
  /* Aktifkan UART Wakeup agar saat ESP32 dalam modem sleep / light sleep,
     sinyal serial dari STM32 langsung membangunkan ESP32 secara instan tanpa kehilangan byte */
  uart_set_wakeup_threshold(UART_PORT_NUM, 3);
  esp_sleep_enable_uart_wakeup(UART_PORT_NUM);

  uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
  char rfid_str[256];
  int rfid_idx = 0;

  esp_task_wdt_add(NULL);

  while (1) {
    esp_task_wdt_reset();
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
            } else if (strncmp(rfid_str, "TXN:", 4) == 0) {
              char *p = rfid_str + 4;
              char *nl = strpbrk(p, "\r\n");
              if (nl) *nl = '\0';
              char *t_uid  = strtok(p, "|");
              char *t_time = strtok(NULL, "|");
              char *t_id   = strtok(NULL, "|");
              char *t_type = strtok(NULL, "|");
              char *t_src  = strtok(NULL, "|");
              if (t_uid && t_time && t_id) {
                send_custom_api_request(t_uid, t_time, t_id, t_type ? t_type : "rfid", t_src ? t_src : "rtc");
              }
            } else if (strncmp(rfid_str, "BULK_START:", 11) == 0) {
              if (s_bulk_transactions_array) {
                cJSON_Delete(s_bulk_transactions_array);
              }
              s_bulk_transactions_array = cJSON_CreateArray();
              ESP_LOGI(TAG, "Bulk transfer started, expected items: %s", rfid_str + 11);
            } else if (strncmp(rfid_str, "BULK_ITEM:", 10) == 0) {
              if (s_bulk_transactions_array) {
                char *p = rfid_str + 10;
                char *nl = strpbrk(p, "\r\n");
                if (nl) *nl = '\0';
                char *t_uid  = strtok(p, "|,");
                char *t_time = strtok(NULL, "|,");
                char *t_id   = strtok(NULL, "|,");
                char *t_type = strtok(NULL, "|,");
                char *t_src  = strtok(NULL, "|,");
                if (t_uid && t_time && t_id) {
                  cJSON *item = cJSON_CreateObject();
                  cJSON_AddStringToObject(item, "transaction_uid", t_uid);
                  cJSON_AddStringToObject(item, "timestamp", t_time);
                  cJSON_AddStringToObject(item, "identifier", t_id);
                  cJSON_AddStringToObject(item, "identifier_type", t_type ? t_type : "rfid");
                  cJSON_AddStringToObject(item, "time_source", t_src ? t_src : "rtc");
                  cJSON_AddItemToArray(s_bulk_transactions_array, item);
                }
              }
            } else if (strncmp(rfid_str, "BULK_END", 8) == 0) {
              if (s_bulk_transactions_array) {
                send_bulk_api_request(s_bulk_transactions_array);
                s_bulk_transactions_array = NULL;
              } else {
                uart_send_and_log("BULK_ACK:0|0\n");
              }
            } else if (strncmp(rfid_str, "CMD:REGISTER_DEVICE:", 20) == 0) {
              char *p = rfid_str + 20;
              char *nl = strpbrk(p, "\r\n");
              if (nl) *nl = '\0';
              char *bar = strchr(p, '|');
              if (bar) {
                *bar = '\0';
                strncpy(s_device_code, p, sizeof(s_device_code) - 1);
                strncpy(s_device_name, bar + 1, sizeof(s_device_name) - 1);
              } else {
                strncpy(s_device_code, p, sizeof(s_device_code) - 1);
              }
              nvs_handle_t my_handle;
              if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
                nvs_set_str(my_handle, "dev_code", s_device_code);
                nvs_set_str(my_handle, "dev_name", s_device_name);
                nvs_commit(my_handle);
                nvs_close(my_handle);
                ESP_LOGI(TAG, "Device config saved: Code='%s', Name='%s'", s_device_code, s_device_name);
              }
              send_register_device_request(s_device_code, s_device_name);
            } else if (strncmp(rfid_str, "CMD:SET_DEVICE:", 15) == 0) {
              char *p = rfid_str + 15;
              char *nl = strpbrk(p, "\r\n");
              if (nl) *nl = '\0';
              char *bar = strchr(p, '|');
              if (bar) {
                *bar = '\0';
                strncpy(s_device_code, p, sizeof(s_device_code) - 1);
                strncpy(s_device_name, bar + 1, sizeof(s_device_name) - 1);
              } else {
                strncpy(s_device_code, p, sizeof(s_device_code) - 1);
              }
              nvs_handle_t my_handle;
              if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
                nvs_set_str(my_handle, "dev_code", s_device_code);
                nvs_set_str(my_handle, "dev_name", s_device_name);
                nvs_commit(my_handle);
                nvs_close(my_handle);
                ESP_LOGI(TAG, "Device config saved: Code='%s', Name='%s'", s_device_code, s_device_name);
              }
              uart_send_and_log("DEVICE_SAVED\n");
            } else if (strncmp(rfid_str, "CMD:SET_SYNC_MODE:", 18) == 0) {
              char *p = rfid_str + 18;
              int mode = atoi(p);
              nvs_handle_t my_handle;
              if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
                nvs_set_i32(my_handle, "sync_mode", mode);
                nvs_commit(my_handle);
                nvs_close(my_handle);
                ESP_LOGI(TAG, "Sync mode saved to NVS: %d", mode);
              }
              uart_send_and_log("SYNC_MODE_SAVED\n");
            } else if (strncmp(rfid_str, "CMD:SYNC_TIME", 13) == 0) {
              EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
              if (!(bits & WIFI_CONNECTED_BIT)) {
                uart_send_and_log("TIME_ERR:WIFI_DISCONNECTED\n");
              } else {
                xSemaphoreGive(sync_time_sem);
              }
            } else if (strncmp(rfid_str, "CMD:GET_WIFI_STATUS", 19) == 0 ||
                       strncmp(rfid_str, "CHECK_WIFI", 10) == 0) {
              EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
              if (bits & WIFI_CONNECTED_BIT) {
                uart_send_and_log("WIFI:OK\n");
              } else {
                uart_send_and_log("WIFI:FAIL\n");
              }
              vTaskDelay(pdMS_TO_TICKS(40));
              send_wifi_status_info();
            } else if (strncmp(rfid_str, "CMD:GET_WIFI_INFO", 17) == 0 ||
                       strncmp(rfid_str, "GET_INFO", 8) == 0) {
              send_wifi_status_info();
            } else if (strncmp(rfid_str, "WIFI:", 5) == 0) {
              char *data = rfid_str + 5;
              /* Bersihkan karakter newline dan carriage return */
              char *nl = strpbrk(data, "\r\n");
              if (nl) *nl = '\0';

              char parsed_ssid[34] = {0};
              char parsed_pass[66] = {0};
              char parsed_ip[16] = "DHCP";

              /* Parse format: WIFI:SSID|PASS|IP atau WIFI:SSID|PASS */
              char *first_bar = strchr(data, '|');
              if (first_bar) {
                *first_bar = '\0';
                strncpy(parsed_ssid, data, sizeof(parsed_ssid) - 1);
                char *rest = first_bar + 1;
                char *second_bar = strchr(rest, '|');
                if (second_bar) {
                  *second_bar = '\0';
                  strncpy(parsed_pass, rest, sizeof(parsed_pass) - 1);
                  strncpy(parsed_ip, second_bar + 1, sizeof(parsed_ip) - 1);
                } else {
                  strncpy(parsed_pass, rest, sizeof(parsed_pass) - 1);
                }
              } else {
                strncpy(parsed_ssid, data, sizeof(parsed_ssid) - 1);
              }

              if (strlen(parsed_ssid) > 0) {
                  nvs_handle_t my_handle;
                  if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
                      nvs_set_str(my_handle, "ssid", parsed_ssid);
                      nvs_set_str(my_handle, "pass", parsed_pass);
                      nvs_set_str(my_handle, "ip", parsed_ip);
                      nvs_commit(my_handle);
                      nvs_close(my_handle);
                      ESP_LOGI(TAG, "WiFi config saved to NVS: SSID='%s', Pass='%s', IP='%s'. Restarting...",
                               parsed_ssid, parsed_pass, parsed_ip);
                  }
                  uart_send_and_log("WIFI_SAVED\n");
                  vTaskDelay(pdMS_TO_TICKS(800));
                  esp_restart();
              }
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
  // Cek apakah restart sebelumnya dipicu oleh Watchdog (Self-Healing Recovery)
  esp_reset_reason_t rst_reason = esp_reset_reason();
  if (rst_reason == ESP_RST_TASK_WDT || rst_reason == ESP_RST_WDT) {
    ESP_LOGW(TAG, "WATCHDOG RESET DETECTED! Microcontroller self-recovered from deadlock/hang.");
  }

  // Inisialisasi Task Watchdog Timer (TWDT)
  esp_task_wdt_config_t twdt_config = {
      .timeout_ms = 8000,          // 8 detik timeout
      .idle_core_mask = (1 << 0),   // pantau idle task core 0
      .trigger_panic = true,        // auto-reboot via hardware panic saat hang
  };
  if (esp_task_wdt_init(&twdt_config) != ESP_OK) {
      esp_task_wdt_reconfigure(&twdt_config);
  }

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
