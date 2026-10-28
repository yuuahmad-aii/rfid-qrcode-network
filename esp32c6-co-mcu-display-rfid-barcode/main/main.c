#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
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
#include "driver/temperature_sensor.h"
#include "esp_timer.h"


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

/* STM32 Bootloader & Reset Control Pins */
#define STM32_NRST_GPIO   GPIO_NUM_22
#define STM32_BOOT0_GPIO  GPIO_NUM_21


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
static esp_netif_t *s_ap_netif = NULL;
static bool s_is_ap_mode = false;
static httpd_handle_t s_http_server = NULL;
static volatile bool s_flashing_stm32_active = false;

/* Cached Wi-Fi info untuk Web Dashboard */
static char s_wifi_ssid_buf[34] = "-";
static char s_wifi_ip_buf[16] = "0.0.0.0";
static char s_wifi_rssi_buf[16] = "- dBm";

/* In-memory Data & Status STM32 untuk Web Dashboard */
typedef struct {
  float temp;
  int rfid_ready;
  char rfid_ver[16];
  int sd_mounted;
  uint32_t pending_count;
  uint32_t uptime_sec;
} stm32_stats_t;

static stm32_stats_t s_stm32_stats = {
    .temp = 32.0f,
    .rfid_ready = 1,
    .rfid_ver = "0x18",
    .sd_mounted = 1,
    .pending_count = 0,
    .uptime_sec = 0
};
static SemaphoreHandle_t s_stm_stats_sem = NULL;

static char s_stm_config_str[256] = {0};
static SemaphoreHandle_t s_stm_config_sem = NULL;
static SemaphoreHandle_t s_stm_config_saved_sem = NULL;

#define MAX_WEB_LINES 120
#define MAX_LINE_LEN 140
typedef struct {
  char lines[MAX_WEB_LINES][MAX_LINE_LEN];
  uint16_t count;
  char filename[32];
  bool in_progress;
  bool is_eof;
} web_stream_buf_t;

static web_stream_buf_t s_log_stream = {0};
static web_stream_buf_t s_user_stream = {0};
static SemaphoreHandle_t s_log_stream_sem = NULL;
static SemaphoreHandle_t s_user_stream_sem = NULL;
static SemaphoreHandle_t s_user_write_sem = NULL;
static bool s_user_write_ok = false;

/* ESP32-C6 On-Chip Temperature Sensor */
static temperature_sensor_handle_t s_temp_sensor = NULL;

static void init_esp32_temp_sensor(void) {
  temperature_sensor_config_t temp_sensor_config =
      TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 80);
  if (temperature_sensor_install(&temp_sensor_config, &s_temp_sensor) == ESP_OK) {
    temperature_sensor_enable(s_temp_sensor);
  }
}

static float get_esp32_temperature(void) {
  float tsens_out = 33.5f;
  if (s_temp_sensor) {
    temperature_sensor_get_celsius(s_temp_sensor, &tsens_out);
  }
  return tsens_out;
}

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
  strncpy(s_wifi_ssid_buf, ssid_str, sizeof(s_wifi_ssid_buf) - 1);
  strncpy(s_wifi_ip_buf, ip_str, sizeof(s_wifi_ip_buf) - 1);
  snprintf(s_wifi_rssi_buf, sizeof(s_wifi_rssi_buf), "%d dBm", rssi);
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

/* ========================================================================== */
/*                STM32 BOOTLOADER FLASHER & HARDWARE CONTROL                 */
/* ========================================================================== */

static void stm32_gpio_init(void) {
    /* Konfigurasi pin kontrol bootloader STM32: IO21 (BOOT0) dan IO22 (NRST) */
    gpio_reset_pin(STM32_BOOT0_GPIO);
    gpio_reset_pin(STM32_NRST_GPIO);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << STM32_NRST_GPIO) | (1ULL << STM32_BOOT0_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    /* Kondisi normal: BOOT0 = 0 (Flash Normal), NRST = 1 (Running) */
    gpio_set_level(STM32_BOOT0_GPIO, 0);
    gpio_set_level(STM32_NRST_GPIO, 1);
}

typedef enum {
    FLASH_STATE_IDLE = 0,
    FLASH_STATE_UPLOADING,
    FLASH_STATE_READY,
    FLASH_STATE_ERASING,
    FLASH_STATE_PROGRAMMING,
    FLASH_STATE_SUCCESS,
    FLASH_STATE_ERROR
} flash_state_t;

static volatile flash_state_t s_flash_state = FLASH_STATE_IDLE;
static volatile int s_flash_progress = 0;
static char s_flash_status_msg[128] = "Standby";
static size_t s_staged_firmware_size = 0;

static esp_err_t stm32_bootloader_enter(void) {
    ESP_LOGI(TAG, "Entering STM32 ROM Bootloader...");

    /* Pastikan pin dikonfigurasi sebagai output aktif */
    gpio_set_direction(STM32_BOOT0_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(STM32_NRST_GPIO, GPIO_MODE_OUTPUT);

    /* 1. Beri tahu STM32 via UART sebelum reset agar menampilkan layar peringatan di LCD */
    uart_send_and_log("OTA_STM32_START\n");
    vTaskDelay(pdMS_TO_TICKS(220));

    /* 2. Tarik BOOT0 = HIGH (Pilih boot dari System Memory / ROM Bootloader) */
    gpio_set_level(STM32_BOOT0_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    /* 3. Tarik NRST = LOW (Reset MCU) */
    gpio_set_level(STM32_NRST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(100));

    /* 4. Lepas NRST = HIGH sementara BOOT0 TETAP HIGH */
    gpio_set_level(STM32_NRST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(300)); /* Tunggu ROM bootloader internal STM32 stabil */

    /* 5. Ganti UART parity ke EVEN (Protokol ST AN3155 mensyaratkan 8-E-1) */
    uart_set_parity(UART_PORT_NUM, UART_PARITY_EVEN);
    vTaskDelay(pdMS_TO_TICKS(20));
    uart_flush_input(UART_PORT_NUM);

    /* 6. Kirim init byte 0x7F untuk auto-baud synchronize dengan retry hingga 10 kali */
    uint8_t init_cmd = 0x7F;
    uint8_t ack = 0;
    for (int retry = 0; retry < 10; retry++) {
        uart_flush_input(UART_PORT_NUM);
        uart_write_bytes(UART_PORT_NUM, &init_cmd, 1);
        int len = uart_read_bytes(UART_PORT_NUM, &ack, 1, pdMS_TO_TICKS(350));
        if (len > 0 && (ack == 0x79 || ack == 0x1F)) {
            ESP_LOGI(TAG, "STM32 Bootloader synchronized! (ack=0x%02X on attempt %d)", ack, retry + 1);
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGE(TAG, "Failed to enter STM32 bootloader! (ack=0x%02X)", ack);
    return ESP_FAIL;
}

static void stm32_bootloader_exit(void) {
    ESP_LOGI(TAG, "Exiting STM32 Bootloader to Normal Application Mode...");
    /* BOOT0 = LOW */
    gpio_set_level(STM32_BOOT0_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(20));

    /* Pulse NRST */
    gpio_set_level(STM32_NRST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(80));
    gpio_set_level(STM32_NRST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(150));

    /* Kembalikan UART ke Normal Parity (Disabled / 8-N-1) */
    uart_set_parity(UART_PORT_NUM, UART_PARITY_DISABLE);
    uart_flush_input(UART_PORT_NUM);
}

static esp_err_t stm32_bootloader_erase(void) {
    ESP_LOGI(TAG, "Erasing STM32 Flash (Extended Erase 0x44)...");
    /* Command Extended Erase: 0x44 0xBB */
    uint8_t cmd[2] = {0x44, 0xBB};
    uart_write_bytes(UART_PORT_NUM, cmd, 2);

    uint8_t ack = 0;
    int len = uart_read_bytes(UART_PORT_NUM, &ack, 1, pdMS_TO_TICKS(1500));
    if (len <= 0 || ack != 0x79) {
        ESP_LOGE(TAG, "Erase command ACK failed: 0x%02X", ack);
        return ESP_FAIL;
    }

    /* Global Mass Erase: 0xFF 0xFF 0x00 */
    uint8_t mass_erase[3] = {0xFF, 0xFF, 0x00};
    uart_write_bytes(UART_PORT_NUM, mass_erase, 3);

    /* Mass erase flash 1MB butuh waktu 3-15 detik */
    len = uart_read_bytes(UART_PORT_NUM, &ack, 1, pdMS_TO_TICKS(30000));
    if (len <= 0 || ack != 0x79) {
        ESP_LOGE(TAG, "Global Mass Erase execution failed: 0x%02X", ack);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "STM32 Flash Erased successfully!");
    return ESP_OK;
}

static esp_err_t stm32_bootloader_write_chunk(uint32_t addr, const uint8_t *data, uint16_t len) {
    if (len == 0 || len > 256) return ESP_ERR_INVALID_ARG;

    /* 1. Kirim Command Write Memory: 0x31 0xCE */
    uint8_t cmd[2] = {0x31, 0xCE};
    uart_write_bytes(UART_PORT_NUM, cmd, 2);
    uint8_t ack = 0;
    int rlen = uart_read_bytes(UART_PORT_NUM, &ack, 1, pdMS_TO_TICKS(1000));
    if (rlen <= 0 || ack != 0x79) {
        ESP_LOGE(TAG, "Write cmd ACK failed at 0x%08lX (ack=0x%02X)", (unsigned long)addr, ack);
        return ESP_FAIL;
    }

    /* 2. Kirim 4 byte alamat (MSB first) + XOR Checksum */
    uint8_t addr_buf[5];
    addr_buf[0] = (uint8_t)((addr >> 24) & 0xFF);
    addr_buf[1] = (uint8_t)((addr >> 16) & 0xFF);
    addr_buf[2] = (uint8_t)((addr >> 8) & 0xFF);
    addr_buf[3] = (uint8_t)(addr & 0xFF);
    addr_buf[4] = addr_buf[0] ^ addr_buf[1] ^ addr_buf[2] ^ addr_buf[3];
    uart_write_bytes(UART_PORT_NUM, addr_buf, 5);

    rlen = uart_read_bytes(UART_PORT_NUM, &ack, 1, pdMS_TO_TICKS(1000));
    if (rlen <= 0 || ack != 0x79) {
        ESP_LOGE(TAG, "Address ACK failed at 0x%08lX (ack=0x%02X)", (unsigned long)addr, ack);
        return ESP_FAIL;
    }

    /* 3. Kirim N-1, data bytes, dan XOR Checksum */
    uint8_t write_buf[258];
    uint8_t count = (uint8_t)(len - 1);
    write_buf[0] = count;
    uint8_t xor_sum = count;
    for (uint16_t i = 0; i < len; i++) {
        write_buf[1 + i] = data[i];
        xor_sum ^= data[i];
    }
    write_buf[1 + len] = xor_sum;

    uart_write_bytes(UART_PORT_NUM, write_buf, len + 2);
    rlen = uart_read_bytes(UART_PORT_NUM, &ack, 1, pdMS_TO_TICKS(2000));
    if (rlen <= 0 || ack != 0x79) {
        ESP_LOGE(TAG, "Data payload ACK failed at 0x%08lX (ack=0x%02X)", (unsigned long)addr, ack);
        return ESP_FAIL;
    }

    return ESP_OK;
}

/* Background Task untuk mem-flash STM32 dari Flash Staging Partition ota_1 */
static void stm32_flasher_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting background STM32 flashing task (Size: %u bytes)...", (unsigned int)s_staged_firmware_size);
    s_flashing_stm32_active = true;
    vTaskDelay(pdMS_TO_TICKS(200));

    const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
    if (!part) {
        ESP_LOGE(TAG, "OTA_1 partition not found for staging!");
        s_flash_state = FLASH_STATE_ERROR;
        snprintf(s_flash_status_msg, sizeof(s_flash_status_msg), "Error: Partisi staging flash tidak ditemukan!");
        s_flashing_stm32_active = false;
        vTaskDelete(NULL);
        return;
    }

    /* 1. Masuk ke Bootloader STM32 */
    s_flash_state = FLASH_STATE_ERASING;
    s_flash_progress = 5;
    snprintf(s_flash_status_msg, sizeof(s_flash_status_msg), "Menghubungkan ke ROM Bootloader STM32...");

    if (stm32_bootloader_enter() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enter STM32 bootloader!");
        s_flash_state = FLASH_STATE_ERROR;
        snprintf(s_flash_status_msg, sizeof(s_flash_status_msg), "Gagal terhubung ke bootloader STM32. Cek koneksi pin IO21 (BOOT0) & IO22 (NRST).");
        s_flashing_stm32_active = false;
        vTaskDelete(NULL);
        return;
    }

    /* 2. Mass Erase Flash STM32 */
    s_flash_progress = 12;
    snprintf(s_flash_status_msg, sizeof(s_flash_status_msg), "Menghapus memori Flash STM32 (Mass Erase)...");
    if (stm32_bootloader_erase() != ESP_OK) {
        stm32_bootloader_exit();
        s_flash_state = FLASH_STATE_ERROR;
        snprintf(s_flash_status_msg, sizeof(s_flash_status_msg), "Gagal melakukan Mass Erase Flash STM32.");
        s_flashing_stm32_active = false;
        vTaskDelete(NULL);
        return;
    }

    /* 3. Tulis firmware dari partisi staging ota_1 ke Flash STM32 */
    s_flash_state = FLASH_STATE_PROGRAMMING;
    uint32_t current_addr = 0x08000000;
    size_t bytes_written = 0;
    uint8_t read_buf[256];

    while (bytes_written < s_staged_firmware_size) {
        size_t chunk_len = sizeof(read_buf);
        if (chunk_len > (s_staged_firmware_size - bytes_written)) {
            chunk_len = s_staged_firmware_size - bytes_written;
        }

        esp_err_t r_err = esp_partition_read(part, bytes_written, read_buf, chunk_len);
        if (r_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed reading staged firmware from flash!");
            stm32_bootloader_exit();
            s_flash_state = FLASH_STATE_ERROR;
            snprintf(s_flash_status_msg, sizeof(s_flash_status_msg), "Gagal membaca file dari partisi staging.");
            s_flashing_stm32_active = false;
            vTaskDelete(NULL);
            return;
        }

        if (stm32_bootloader_write_chunk(current_addr, read_buf, (uint16_t)chunk_len) != ESP_OK) {
            ESP_LOGE(TAG, "Write chunk failed at 0x%08lX", (unsigned long)current_addr);
            stm32_bootloader_exit();
            s_flash_state = FLASH_STATE_ERROR;
            snprintf(s_flash_status_msg, sizeof(s_flash_status_msg), "Gagal memprogram sektor flash 0x%08lX.", (unsigned long)current_addr);
            s_flashing_stm32_active = false;
            vTaskDelete(NULL);
            return;
        }

        current_addr += chunk_len;
        bytes_written += chunk_len;

        int pct = 15 + (int)((bytes_written * 83) / s_staged_firmware_size);
        s_flash_progress = pct;
        snprintf(s_flash_status_msg, sizeof(s_flash_status_msg), "Memprogram Flash STM32: %d%% (%u / %u KB)...", pct, (unsigned int)(bytes_written / 1024), (unsigned int)(s_staged_firmware_size / 1024));

        /* Beri nafas untuk RTOS */
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    /* 4. Keluar dari bootloader dan jalankan STM32 secara normal */
    stm32_bootloader_exit();
    s_flashing_stm32_active = false;

    s_flash_progress = 100;
    s_flash_state = FLASH_STATE_SUCCESS;
    snprintf(s_flash_status_msg, sizeof(s_flash_status_msg), "SUKSES: Firmware STM32 berhasil di-flash! STM32 telah reboot.");

    vTaskDelay(pdMS_TO_TICKS(200));
    uart_send_and_log("OTA_PROGRESS:100|Pembaruan selesai! Menjalankan program baru...\n");
    vTaskDelay(pdMS_TO_TICKS(500));
    uart_send_and_log("STA_ACTIVE\n");

    ESP_LOGI(TAG, "STM32 Flashing background task completed successfully!");
    vTaskDelete(NULL);
}

/* ========================================================================== */
/*                   WEB SERVER PORTAL HTML & CSS TEMPLATE                    */
/* ========================================================================== */

static const char ota_html_page[] =
"<!DOCTYPE html>"
"<html lang=\"id\">"
"<head>"
"<meta charset=\"UTF-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">"
"<title>Web Dashboard Gateway Absensi RFID</title>"
"<style>"
":root{--bg:#0b1120;--card:#1e293b;--border:#334155;--cyan:#06b6d4;--cyan-h:#0891b2;--amber:#f59e0b;--red:#ef4444;--green:#10b981;--purple:#a855f7;--text:#f8fafc;--muted:#94a3b8;}"
"*{box-sizing:border-box;margin:0;padding:0;font-family:-apple-system,BlinkMacSystemFont,\"Segoe UI\",Roboto,sans-serif;}"
"body{background:var(--bg);color:var(--text);padding:14px;min-height:100vh;}"
".wrap{max-width:920px;margin:0 auto;}"
".hdr{display:flex;justify-content:space-between;align-items:center;border-bottom:2px solid var(--border);padding-bottom:12px;margin-bottom:14px;flex-wrap:wrap;gap:10px;}"
".hdr h1{font-size:19px;font-weight:700;color:#fff;}"
".hdr p{font-size:12px;color:var(--muted);margin-top:2px;}"
".badge{background:#064e3b;color:#34d399;border:1px solid #059669;padding:4px 10px;border-radius:9999px;font-size:11px;font-weight:600;}"
".nav-tabs{display:flex;gap:6px;margin-bottom:16px;border-bottom:1px solid var(--border);padding-bottom:10px;overflow-x:auto;}"
".tab-btn{background:transparent;border:1px solid transparent;color:var(--muted);padding:8px 14px;border-radius:8px;font-size:12px;font-weight:600;cursor:pointer;white-space:nowrap;transition:0.2s;}"
".tab-btn:hover{color:#fff;background:rgba(255,255,255,0.05);}"
".tab-btn.active{background:var(--cyan);color:#082f49;}"
".tab-panel{display:none;}"
".tab-panel.active{display:block;}"
".grid-2{display:grid;grid-template-columns:1fr;gap:14px;margin-bottom:14px;}"
"@media(min-width:720px){.grid-2{grid-template-columns:1fr 1fr;}}"
".grid-3{display:grid;grid-template-columns:1fr;gap:12px;margin-bottom:14px;}"
"@media(min-width:720px){.grid-3{grid-template-columns:1fr 1fr 1fr;}}"
".card{background:var(--card);border:1px solid var(--border);border-radius:10px;padding:15px;margin-bottom:14px;}"
".card h3{font-size:13px;font-weight:600;color:var(--cyan);margin-bottom:12px;display:flex;align-items:center;gap:6px;}"
".stat-row{display:flex;justify-content:space-between;align-items:center;padding:7px 0;border-bottom:1px solid rgba(255,255,255,0.05);font-size:12px;}"
".stat-row:last-child{border-bottom:none;}"
".stat-lbl{color:var(--muted);}"
".stat-val{font-weight:600;color:#fff;}"
".val-grn{color:var(--green);}"
".val-amb{color:var(--amber);}"
".val-red{color:var(--red);}"
".val-cyn{color:var(--cyan);}"
".form-group{margin-bottom:11px;}"
".form-group label{display:block;font-size:11px;color:var(--muted);margin-bottom:4px;font-weight:500;}"
".form-control{width:100%;background:#0f172a;border:1px solid var(--border);border-radius:6px;padding:8px 10px;color:#fff;font-size:12px;outline:none;}"
".form-control:focus{border-color:var(--cyan);}"
".btn{display:inline-flex;align-items:center;justify-content:center;gap:6px;padding:9px 14px;border:none;border-radius:6px;font-size:12px;font-weight:600;cursor:pointer;transition:0.2s;}"
".btn-pri{background:var(--cyan);color:#082f49;}"
".btn-pri:hover:not(:disabled){background:var(--cyan-h);}"
".btn-sec{background:#334155;color:#fff;}"
".btn-sec:hover{background:#475569;}"
".btn-grn{background:var(--green);color:#064e3b;}"
".btn-grn:hover{background:#059669;}"
".btn-w100{width:100%;}"
".btn:disabled{opacity:0.5;cursor:not-allowed;}"
".tbl-wrap{overflow-x:auto;max-height:360px;margin-top:10px;border:1px solid var(--border);border-radius:6px;}"
"table{width:100%;border-collapse:collapse;font-size:11px;}"
"th{background:#0f172a;padding:8px 10px;text-align:left;color:var(--muted);font-weight:600;position:sticky;top:0;}"
"td{padding:8px 10px;border-top:1px solid rgba(255,255,255,0.05);color:#cbd5e1;}"
"tr:hover{background:rgba(255,255,255,0.02);}"
".tag{display:inline-block;padding:2px 7px;border-radius:4px;font-size:10px;font-weight:600;}"
".tag-masuk{background:#064e3b;color:#34d399;}"
".tag-pulang{background:#78350f;color:#fcd34d;}"
".tag-istirahat{background:#164e63;color:#67e8f9;}"
".tag-izin{background:#581c87;color:#d8b4fe;}"
".toast{padding:10px 14px;border-radius:7px;font-size:12px;margin-top:12px;display:none;line-height:1.4;}"
".toast.ok{background:rgba(16,185,129,0.15);border:1px solid var(--green);color:#6ee7b7;}"
".toast.err{background:rgba(239,68,68,0.15);border:1px solid var(--red);color:#fca5a5;}"
".pwrap{margin-top:10px;display:none;}"
".pbar{background:#0f172a;height:8px;border-radius:9999px;overflow:hidden;}"
".pfill{background:linear-gradient(90deg,#06b6d4,#10b981);height:100%;width:0%;transition:width 0.25s;}"
".ptxt{display:flex;justify-content:space-between;font-size:10px;color:var(--muted);margin-top:4px;}"
".fbox{border:2px dashed var(--border);border-radius:8px;padding:12px;text-align:center;cursor:pointer;margin-bottom:10px;font-size:11px;color:var(--muted);}"
".fbox:hover{border-color:var(--cyan);background:rgba(6,182,212,0.06);}"
".fbox input{display:none;}"
".warn-box{background:rgba(245,158,11,0.12);border:1px solid var(--amber);border-radius:8px;padding:10px 14px;margin-bottom:14px;font-size:11px;color:#fde68a;display:flex;gap:10px;align-items:center;}"
"</style>"
"</head>"
"<body>"
"<div class=\"wrap\">"
"<div class=\"hdr\">"
"<div><h1>Gateway Absensi RFID &bull; Web Dashboard</h1><p>Mode SoftAP Admin &bull; IP: 192.168.4.1</p></div>"
"<span class=\"badge\">&#9679; SoftAP Siap</span>"
"</div>"
"<div class=\"nav-tabs\">"
"<button class=\"tab-btn active\" onclick=\"showTab('stats')\">&#128202; Statistik</button>"
"<button class=\"tab-btn\" onclick=\"showTab('logs')\">&#128197; Log Harian</button>"
"<button class=\"tab-btn\" onclick=\"showTab('users')\">&#128101; Karyawan</button>"
"<button class=\"tab-btn\" onclick=\"showTab('settings')\">&#9881; Pengaturan</button>"
"<button class=\"tab-btn\" onclick=\"showTab('ota')\">&#128640; Update OTA</button>"
"</div>"

/* TAB 1: STATISTIK */
"<div class=\"tab-panel active\" id=\"panel-stats\">"
"<div class=\"grid-2\">"
"<div class=\"card\">"
"<h3>&#127777; Suhu Mikrokontroler</h3>"
"<div class=\"stat-row\"><span class=\"stat-lbl\">Suhu STM32 Main MCU</span><span class=\"stat-val val-cyn\" id=\"st-stm-temp\">-- &deg;C</span></div>"
"<div class=\"stat-row\"><span class=\"stat-lbl\">Suhu ESP32-C6 Co-MCU</span><span class=\"stat-val val-cyn\" id=\"st-esp-temp\">-- &deg;C</span></div>"
"<div class=\"stat-row\"><span class=\"stat-lbl\">Status Termal</span><span class=\"stat-val val-grn\" id=\"st-thermal-st\">Normal (Aman)</span></div>"
"</div>"
"<div class=\"card\">"
"<h3>&#128246; Koneksi Jaringan Wi-Fi</h3>"
"<div class=\"stat-row\"><span class=\"stat-lbl\">Status Wi-Fi</span><span class=\"stat-val\" id=\"st-wifi-st\">--</span></div>"
"<div class=\"stat-row\"><span class=\"stat-lbl\">SSID Target</span><span class=\"stat-val\" id=\"st-wifi-ssid\">--</span></div>"
"<div class=\"stat-row\"><span class=\"stat-lbl\">Kekuatan Sinyal (RSSI)</span><span class=\"stat-val\" id=\"st-wifi-rssi\">--</span></div>"
"<div class=\"stat-row\"><span class=\"stat-lbl\">Mode Gateway</span><span class=\"stat-val val-amb\" id=\"st-gateway-mode\">SoftAP (192.168.4.1)</span></div>"
"</div>"
"</div>"
"<div class=\"grid-2\">"
"<div class=\"card\">"
"<h3>&#128279; Periferal Hardware & Storage</h3>"
"<div class=\"stat-row\"><span class=\"stat-lbl\">RFID Reader CLRC663</span><span class=\"stat-val\" id=\"st-rfid-st\">--</span></div>"
"<div class=\"stat-row\"><span class=\"stat-lbl\">MicroSD Card FatFS</span><span class=\"stat-val\" id=\"st-sd-st\">--</span></div>"
"<div class=\"stat-row\"><span class=\"stat-lbl\">Antrean Pending Sync</span><span class=\"stat-val val-amb\" id=\"st-pending\">-- data</span></div>"
"</div>"
"<div class=\"card\">"
"<h3>&#9201; Uptime & Konsumsi Memori</h3>"
"<div class=\"stat-row\"><span class=\"stat-lbl\">Uptime STM32</span><span class=\"stat-val\" id=\"st-stm-up\">--</span></div>"
"<div class=\"stat-row\"><span class=\"stat-lbl\">Uptime ESP32</span><span class=\"stat-val\" id=\"st-esp-up\">--</span></div>"
"<div class=\"stat-row\"><span class=\"stat-lbl\">Free RAM Heap ESP32</span><span class=\"stat-val val-grn\" id=\"st-esp-heap\">-- KB</span></div>"
"</div>"
"</div>"
"<div style=\"display:flex;gap:10px;align-items:center;\">"
"<button class=\"btn btn-pri\" onclick=\"fetchStats()\">&#8635; Segarkan Statistik</button>"
"<label style=\"font-size:12px;color:var(--muted);display:flex;align-items:center;gap:6px;\">"
"<input type=\"checkbox\" id=\"auto-poll\" onchange=\"toggleAutoPoll(this)\"> Auto Refresh (5s)"
"</label>"
"</div>"
"</div>"

/* TAB 2: LOG HARIAN */
"<div class=\"tab-panel\" id=\"panel-logs\">"
"<div class=\"card\">"
"<h3>&#128197; Unduh & Pratinjau Log Absensi Harian</h3>"
"<div style=\"display:flex;gap:10px;flex-wrap:wrap;align-items:flex-end;margin-bottom:12px;\">"
"<div style=\"flex-grow:1;min-width:180px;\">"
"<label style=\"font-size:11px;color:var(--muted);display:block;margin-bottom:4px;\">Tanggal Log (Format YYYYMMDD):</label>"
"<input type=\"text\" class=\"form-control\" id=\"log-date-inp\" placeholder=\"20261028\">"
"</div>"
"<button class=\"btn btn-pri\" onclick=\"loadLogPreview()\">&#128269; Pratinjau Log</button>"
"<button class=\"btn btn-grn\" onclick=\"downloadDailyLog()\">&#128229; Unduh CSV</button>"
"</div>"
"<p style=\"font-size:11px;color:var(--muted);margin-bottom:8px;\" id=\"log-status-lbl\">Menampilkan data dari file log harian...</p>"
"<div class=\"tbl-wrap\">"
"<table>"
"<thead><tr><th>No</th><th>Waktu</th><th>UID Kartu</th><th>Nama Karyawan</th><th>Mode Kehadiran</th></tr></thead>"
"<tbody id=\"log-tbody\"><tr><td colspan=\"5\" style=\"text-align:center;color:var(--muted);padding:20px;\">Tekan tombol Pratinjau Log untuk menampilkan data.</td></tr></tbody>"
"</table>"
"</div>"
"</div>"
"</div>"

/* TAB 3: DAFTAR KARYAWAN */
"<div class=\"tab-panel\" id=\"panel-users\">"
"<div class=\"grid-2\">"
"<div class=\"card\">"
"<h3>&#128229; Unduh Database USERS.CSV</h3>"
"<p style=\"font-size:12px;color:var(--muted);margin-bottom:14px;line-height:1.5;\">Unduh file daftar master karyawan lokal dari slot MicroSD mesin absensi untuk backup atau diedit di spreadsheet.</p>"
"<button class=\"btn btn-grn btn-w100\" onclick=\"window.location.href='/api/users/download'\">&#128229; Unduh File USERS.CSV</button>"
"<button class=\"btn btn-sec btn-w100\" style=\"margin-top:8px;\" onclick=\"loadUsersPreview()\">&#128203; Lihat Tabel di Browser</button>"
"</div>"
"<div class=\"card\">"
"<h3>&#128228; Upload Database USERS.CSV</h3>"
"<p style=\"font-size:12px;color:var(--muted);margin-bottom:10px;line-height:1.5;\">Pilih file <b>USERS.CSV</b> baru untuk ditransfer ke MicroSD mesin absensi. Cache RAM akan otomatis dihangatkan.</p>"
"<div class=\"fbox\" onclick=\"document.getElementById('f-users').click()\">"
"<input type=\"file\" id=\"f-users\" accept=\".csv\" onchange=\"onUserFileSelected(this)\">"
"<div id=\"lbl-users-file\">&#128190; Pilih File <b>USERS.CSV</b></div>"
"</div>"
"<button class=\"btn btn-pri btn-w100\" id=\"btn-up-users\" disabled onclick=\"uploadUsersFile()\">&#9654; Upload ke MicroSD</button>"
"<div class=\"toast\" id=\"t-users\"></div>"
"</div>"
"</div>"
"<div class=\"card\">"
"<h3>&#128101; Pratinjau Daftar Karyawan Terdaftar (USERS.CSV)</h3>"
"<div class=\"tbl-wrap\">"
"<table>"
"<thead><tr><th>No</th><th>UID Kartu (Hex)</th><th>Nama Karyawan</th><th>Departemen / Jabatan</th></tr></thead>"
"<tbody id=\"users-tbody\"><tr><td colspan=\"4\" style=\"text-align:center;color:var(--muted);padding:20px;\">Tekan tombol Lihat Tabel untuk menampilkan daftar karyawan.</td></tr></tbody>"
"</table>"
"</div>"
"</div>"
"</div>"

/* TAB 4: PENGATURAN ADMIN */
"<div class=\"tab-panel\" id=\"panel-settings\">"
"<div class=\"card\">"
"<h3>&#9881; Konfigurasi Parameter Mesin Absensi</h3>"
"<div class=\"grid-3\">"
"<div>"
"<h4 style=\"font-size:12px;color:var(--cyan);margin-bottom:8px;\">Identitas Mesin</h4>"
"<div class=\"form-group\"><label>Kode Perangkat:</label><input type=\"text\" class=\"form-control\" id=\"cfg-code\"></div>"
"<div class=\"form-group\"><label>Nama Perangkat:</label><input type=\"text\" class=\"form-control\" id=\"cfg-name\"></div>"
"<div class=\"form-group\"><label>PIN Admin (6 digit):</label><input type=\"password\" class=\"form-control\" id=\"cfg-pin\" maxlength=\"6\"></div>"
"</div>"
"<div>"
"<h4 style=\"font-size:12px;color:var(--cyan);margin-bottom:8px;\">Jaringan & Sinkronisasi</h4>"
"<div class=\"form-group\"><label>SSID Wi-Fi:</label><input type=\"text\" class=\"form-control\" id=\"cfg-ssid\"></div>"
"<div class=\"form-group\"><label>Password Wi-Fi:</label><input type=\"password\" class=\"form-control\" id=\"cfg-pass\"></div>"
"<div class=\"form-group\"><label>Mode Sinkronisasi:</label>"
"<select class=\"form-control\" id=\"cfg-sync\"><option value=\"0\">Realtime</option><option value=\"1\">Batch</option><option value=\"2\">Periodic</option></select>"
"</div>"
"<div class=\"form-group\"><label>Interval Sinkron (Menit):</label><input type=\"number\" class=\"form-control\" id=\"cfg-sync-int\" min=\"1\" max=\"1440\"></div>"
"</div>"
"<div>"
"<h4 style=\"font-size:12px;color:var(--cyan);margin-bottom:8px;\">Anti-Passback & Layar</h4>"
"<div class=\"form-group\"><label>Anti-Passback Cooldown:</label>"
"<select class=\"form-control\" id=\"cfg-apb\">"
"<option value=\"0\">Nonaktif (0s)</option><option value=\"15\">15 Detik</option><option value=\"30\">30 Detik</option>"
"<option value=\"60\">60 Detik (1 Menit)</option><option value=\"120\">2 Menit</option><option value=\"300\">5 Menit</option><option value=\"600\">10 Menit</option>"
"</select>"
"</div>"
"<div class=\"form-group\"><label>Kecerahan LCD (%):</label><input type=\"number\" class=\"form-control\" id=\"cfg-bright\" min=\"10\" max=\"100\"></div>"
"<div class=\"form-group\"><label>Timeout Layar Sleep:</label>"
"<select class=\"form-control\" id=\"cfg-sleep\">"
"<option value=\"0\">Selalu Aktif (0s)</option><option value=\"15\">15 Detik</option><option value=\"30\">30 Detik</option>"
"<option value=\"60\">1 Menit</option><option value=\"120\">2 Menit</option><option value=\"300\">5 Menit</option>"
"</select>"
"</div>"
"<div class=\"form-group\"><label>Tema Tampilan GUI:</label>"
"<select class=\"form-control\" id=\"cfg-theme\">"
"<option value=\"0\">Dark Cyan</option><option value=\"1\">Midnight Blue</option><option value=\"2\">Forest Green</option>"
"<option value=\"3\">Sunset Amber</option><option value=\"4\">Ruby Dark</option>"
"</select>"
"</div>"
"</div>"
"</div>"
"<div style=\"display:flex;gap:10px;align-items:center;\">"
"<button class=\"btn btn-grn\" onclick=\"saveSettings()\">&#128190; Simpan Semua Pengaturan</button>"
"<button class=\"btn btn-sec\" onclick=\"loadSettings()\">&#8635; Muat Ulang</button>"
"</div>"
"<div class=\"toast\" id=\"t-cfg\"></div>"
"</div>"
"</div>"

/* TAB 5: UPDATE OTA */
"<div class=\"tab-panel\" id=\"panel-ota\">"
"<div class=\"warn-box\"><div>&#9888;</div><div><b>PERINGATAN OPERATOR:</b> Jangan mencabut daya mesin saat flashing berlangsung! Tunggu hingga proses 100% sukses.</div></div>"
"<div class=\"grid-2\">"
"<div class=\"card\">"
"<div style=\"display:flex;justify-content:space-between;margin-bottom:10px;\"><h3>STM32 Main MCU</h3><span class=\"badge\" style=\"background:#1e3a8a;color:#93c5fd;\">STM32F405</span></div>"
"<p style=\"font-size:12px;color:var(--muted);margin-bottom:12px;\">Firmware UI LVGL, RFID Reader, dan penyimpanan MicroSD. Flash via UART ROM Bootloader.</p>"
"<div class=\"fbox\" onclick=\"document.getElementById('f-s').click()\">"
"<input type=\"file\" id=\"f-s\" accept=\".bin\" onchange=\"onF(this,'l-s','b-s')\">"
"<div id=\"l-s\">&#128190; Pilih File <b>firmware_stm32.bin</b></div>"
"</div>"
"<button class=\"btn btn-pri btn-w100\" id=\"b-s\" disabled onclick=\"upF('stm32')\">&#9654; Mulai Update STM32</button>"
"<div class=\"pwrap\" id=\"pw-stm32\"><div class=\"pbar\"><div class=\"pfill\" id=\"pf-stm32\"></div></div>"
"<div class=\"ptxt\"><span id=\"ps-stm32\">Mengunggah...</span><span id=\"pp-stm32\">0%</span></div></div>"
"<div class=\"toast\" id=\"t-stm32\"></div>"
"</div>"
"<div class=\"card\">"
"<div style=\"display:flex;justify-content:space-between;margin-bottom:10px;\"><h3>ESP32-C6 Co-MCU</h3><span class=\"badge\" style=\"background:#3b0764;color:#d8b4fe;\">Wi-Fi 6</span></div>"
"<p style=\"font-size:12px;color:var(--muted);margin-bottom:12px;\">Firmware Wi-Fi 6, HTTP Sync, dan Web Server. Update via native dual-partition OTA.</p>"
"<div class=\"fbox\" onclick=\"document.getElementById('f-e').click()\">"
"<input type=\"file\" id=\"f-e\" accept=\".bin\" onchange=\"onF(this,'l-e','b-e')\">"
"<div id=\"l-e\">&#128190; Pilih File <b>firmware_esp32.bin</b></div>"
"</div>"
"<button class=\"btn btn-pri btn-w100\" id=\"b-e\" disabled onclick=\"upF('esp32')\">&#9654; Mulai Update ESP32</button>"
"<div class=\"pwrap\" id=\"pw-esp32\"><div class=\"pbar\"><div class=\"pfill\" id=\"pf-esp32\"></div></div>"
"<div class=\"ptxt\"><span id=\"ps-esp32\">Mengunggah...</span><span id=\"pp-esp32\">0%</span></div></div>"
"<div class=\"toast\" id=\"t-esp32\"></div>"
"</div>"
"</div>"
"</div>"

"<button class=\"btn btn-sec btn-w100\" style=\"margin-top:14px;\" onclick=\"exitSta()\">&#10005; Tutup Portal & Kembali ke Mode Client (Station)</button>"
"</div>"

"<script>"
"function showTab(id){"
"  document.querySelectorAll('.tab-btn').forEach(function(b){b.classList.remove('active');});"
"  document.querySelectorAll('.tab-panel').forEach(function(p){p.classList.remove('active');});"
"  event.target.classList.add('active');"
"  var p=document.getElementById('panel-'+id);if(p)p.classList.add('active');"
"  if(id==='stats')fetchStats();"
"  if(id==='settings')loadSettings();"
"}"
"function fetchStats(){"
"  fetch('/api/stats').then(function(r){return r.json();}).then(function(d){"
"    if(d.stm32){"
"      document.getElementById('st-stm-temp').innerText=d.stm32.temp.toFixed(1)+' \u00B0C';"
"      document.getElementById('st-rfid-st').innerHTML=d.stm32.rfid_ready?'<span class=\"val-grn\">Online (Ready)</span>':'<span class=\"val-red\">Offline</span>';"
"      document.getElementById('st-sd-st').innerHTML=d.stm32.sd_mounted?'<span class=\"val-grn\">Terpasang (FAT32)</span>':'<span class=\"val-red\">Tidak Terpasang</span>';"
"      document.getElementById('st-pending').innerText=d.stm32.pending_count+' data';"
"      var sm=Math.floor(d.stm32.uptime/60),ss=d.stm32.uptime%60;document.getElementById('st-stm-up').innerText=sm+'m '+ss+'s';"
"    }"
"    if(d.esp32){"
"      document.getElementById('st-esp-temp').innerText=d.esp32.temp.toFixed(1)+' \u00B0C';"
"      document.getElementById('st-wifi-st').innerHTML=d.esp32.wifi_status==='Connected'?'<span class=\"val-grn\">Terhubung</span>':'<span class=\"val-red\">Terputus</span>';"
"      document.getElementById('st-wifi-ssid').innerText=d.esp32.wifi_ssid||'-';"
"      document.getElementById('st-wifi-rssi').innerText=d.esp32.wifi_rssi||'-';"
"      var em=Math.floor(d.esp32.uptime/60),es=d.esp32.uptime%60;document.getElementById('st-esp-up').innerText=em+'m '+es+'s';"
"      document.getElementById('st-esp-heap').innerText=(d.esp32.free_heap/1024).toFixed(1)+' KB';"
"    }"
"  }).catch(function(e){console.log('Stats error',e);});"
"}"
"var pollTimer=null;"
"function toggleAutoPoll(cb){if(cb.checked){pollTimer=setInterval(fetchStats,5000);}else{clearInterval(pollTimer);pollTimer=null;}}"
"function loadLogPreview(){"
"  var dt=document.getElementById('log-date-inp').value.trim();"
"  var url='/api/log/preview'+(dt?('?date='+dt):'');"
"  document.getElementById('log-status-lbl').innerText='Memuat data log dari MicroSD...';"
"  fetch(url).then(function(r){return r.json();}).then(function(d){"
"    var tb=document.getElementById('log-tbody');tb.innerHTML='';"
"    document.getElementById('log-status-lbl').innerText='File: '+d.filename+' ('+d.count+' baris terekam)';"
"    if(!d.lines||d.lines.length===0){tb.innerHTML='<tr><td colspan=\"5\" style=\"text-align:center;padding:20px;\">Tidak ada catatan log pada tanggal ini.</td></tr>';return;}"
"    d.lines.forEach(function(l,i){"
"      var pts=l.split(',');var time='-',uid='-',name='-',mode='Masuk';"
"      pts.forEach(function(p){"
"        if(p.indexOf('UID:')===0)uid=p.substr(4);"
"        else if(p.indexOf('NAME=')===0)name=p.substr(5);"
"        else if(p.indexOf('MODE=')===0)mode=p.substr(5);"
"        else if(p.indexOf('20')===0)time=p;"
"      });"
"      var mcls='tag-masuk';"
"      if(mode.indexOf('Pulang')>=0)mcls='tag-pulang';"
"      else if(mode.indexOf('Istirahat')>=0)mcls='tag-istirahat';"
"      else if(mode.indexOf('Izin')>=0)mcls='tag-izin';"
"      var tr='<tr><td>'+(i+1)+'</td><td>'+time+'</td><td><code>'+uid+'</code></td><td><b>'+name+'</b></td><td><span class=\"tag '+mcls+'\">'+mode+'</span></td></tr>';"
"      tb.innerHTML+=tr;"
"    });"
"  }).catch(function(e){alert('Gagal membaca log: '+e);});"
"}"
"function downloadDailyLog(){"
"  var dt=document.getElementById('log-date-inp').value.trim();"
"  window.location.href='/api/log/download'+(dt?('?date='+dt):'');"
"}"
"function onUserFileSelected(inp){"
"  if(inp.files&&inp.files[0]){"
"    var f=inp.files[0];document.getElementById('lbl-users-file').innerHTML='<b>'+f.name+'</b> ('+(f.size/1024).toFixed(1)+' KB)';"
"    document.getElementById('btn-up-users').disabled=false;"
"  }"
"}"
"function uploadUsersFile(){"
"  var fi=document.getElementById('f-users');if(!fi.files||!fi.files[0])return;"
"  var f=fi.files[0];var tt=document.getElementById('t-users');var b=document.getElementById('btn-up-users');"
"  b.disabled=true;tt.style.display='none';"
"  var x=new XMLHttpRequest();x.open('POST','/api/users/upload',true);"
"  x.onload=function(){"
"    b.disabled=false;"
"    if(x.status===200){tt.className='toast ok';tt.innerText='SUKSES: '+x.responseText;tt.style.display='block';loadUsersPreview();}"
"    else{tt.className='toast err';tt.innerText='GAGAL: '+x.responseText;tt.style.display='block';}"
"  };"
"  x.onerror=function(){b.disabled=false;tt.className='toast err';tt.innerText='GAGAL: Terjadi error koneksi!';tt.style.display='block';};"
"  x.send(f);"
"}"
"function loadUsersPreview(){"
"  fetch('/api/users/preview').then(function(r){return r.json();}).then(function(d){"
"    var tb=document.getElementById('users-tbody');tb.innerHTML='';"
"    if(!d.lines||d.lines.length===0){tb.innerHTML='<tr><td colspan=\"4\" style=\"text-align:center;padding:20px;\">USERS.CSV kosong atau belum dibuat.</td></tr>';return;}"
"    d.lines.forEach(function(l,i){"
"      var p=l.split(',');var uid=p[0]||'-',name=p[1]||'-',dept=p[2]||'-';"
"      tb.innerHTML+='<tr><td>'+(i+1)+'</td><td><code>'+uid+'</code></td><td><b>'+name+'</b></td><td>'+dept+'</td></tr>';"
"    });"
"  }).catch(function(e){alert('Gagal memuat karyawan: '+e);});"
"}"
"function loadSettings(){"
"  fetch('/api/settings').then(function(r){return r.json();}).then(function(d){"
"    document.getElementById('cfg-code').value=d.dev_code||'';"
"    document.getElementById('cfg-name').value=d.dev_name||'';"
"    document.getElementById('cfg-pin').value=d.admin_pin||'';"
"    document.getElementById('cfg-ssid').value=d.wifi_ssid||'';"
"    document.getElementById('cfg-pass').value=d.wifi_pass||'';"
"    document.getElementById('cfg-sync').value=d.sync_mode!==undefined?d.sync_mode:0;"
"    document.getElementById('cfg-sync-int').value=d.sync_interval!==undefined?d.sync_interval:15;"
"    document.getElementById('cfg-apb').value=d.antipassback!==undefined?d.antipassback:60;"
"    document.getElementById('cfg-bright').value=d.brightness!==undefined?d.brightness:80;"
"    document.getElementById('cfg-sleep').value=d.sleep_timeout!==undefined?d.sleep_timeout:30;"
"    document.getElementById('cfg-theme').value=d.theme!==undefined?d.theme:0;"
"  }).catch(function(e){console.log('Settings error',e);});"
"}"
"function saveSettings(){"
"  var payload={"
"    dev_code:document.getElementById('cfg-code').value,"
"    dev_name:document.getElementById('cfg-name').value,"
"    admin_pin:document.getElementById('cfg-pin').value,"
"    wifi_ssid:document.getElementById('cfg-ssid').value,"
"    wifi_pass:document.getElementById('cfg-pass').value,"
"    sync_mode:parseInt(document.getElementById('cfg-sync').value),"
"    sync_interval:parseInt(document.getElementById('cfg-sync-int').value),"
"    antipassback:parseInt(document.getElementById('cfg-apb').value),"
"    brightness:parseInt(document.getElementById('cfg-bright').value),"
"    sleep_timeout:parseInt(document.getElementById('cfg-sleep').value),"
"    theme:parseInt(document.getElementById('cfg-theme').value)"
"  };"
"  var tt=document.getElementById('t-cfg');"
"  fetch('/api/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)}).then(function(r){return r.json();}).then(function(d){"
"    tt.className='toast ok';tt.innerText=d.message||'Pengaturan tersimpan!';tt.style.display='block';"
"  }).catch(function(e){tt.className='toast err';tt.innerText='Gagal menyimpan pengaturan: '+e;tt.style.display='block';});"
"}"
"function onF(i,l,b){if(i.files&&i.files[0]){var f=i.files[0];document.getElementById(l).innerHTML='<b>'+f.name+'</b> ('+(f.size/1024).toFixed(1)+' KB)';document.getElementById(b).disabled=false;}}"
"var otaPoll=null;"
"function pollSTM32(){"
"  fetch('/api/flash_status').then(function(r){return r.json();}).then(function(d){"
"    var pf=document.getElementById('pf-stm32'),pp=document.getElementById('pp-stm32'),ps=document.getElementById('ps-stm32'),tt=document.getElementById('t-stm32'),b=document.getElementById('b-s');"
"    if(d.progress!==undefined){pf.style.width=d.progress+'%';pp.innerText=d.progress+'%';}"
"    if(d.message){ps.innerText=d.message;}"
"    if(d.state==='success'){"
"      clearInterval(otaPoll);"
"      pf.style.width='100%';pp.innerText='100%';ps.innerText='Selesai!';"
"      tt.className='toast ok';tt.innerText=d.message;tt.style.display='block';"
"    }else if(d.state==='error'){"
"      clearInterval(otaPoll);"
"      tt.className='toast err';tt.innerText='GAGAL: '+d.message;tt.style.display='block';b.disabled=false;"
"    }"
"  }).catch(function(e){console.log('Poll error',e);});"
"}"
"function upF(t){"
"  var fi=document.getElementById(t==='stm32'?'f-s':'f-e');if(!fi.files||!fi.files[0])return;var f=fi.files[0];"
"  if(!confirm('Flash '+t.toUpperCase()+' dengan file \"'+f.name+'\"?\\n\\nPERINGATAN: Jangan mematikan daya mesin!'))return;"
"  var b=document.getElementById(t==='stm32'?'b-s':'b-e'),pw=document.getElementById('pw-'+t),pf=document.getElementById('pf-'+t),pp=document.getElementById('pp-'+t),ps=document.getElementById('ps-'+t),tt=document.getElementById('t-'+t);"
"  b.disabled=true;pw.style.display='block';tt.style.display='none';pf.style.width='0%';pp.innerText='0%';ps.innerText='Mengunggah file ke gateway...';"
"  var x=new XMLHttpRequest();x.open('POST','/update/'+t,true);"
"  x.upload.onprogress=function(e){if(e.lengthComputable){var p=Math.round((e.loaded/e.total)*(t==='stm32'?10:90));pf.style.width=p+'%';pp.innerText=p+'%';if(t==='stm32'&&p>=10)ps.innerText='File tersimpan di gateway. Mempersiapkan flash STM32...';}};"
"  x.onload=function(){"
"    if(x.status===200){"
"      if(t==='stm32'){"
"        ps.innerText='Memulai penghapusan & pemrograman flash STM32...';"
"        otaPoll=setInterval(pollSTM32,600);"
"      }else{"
"        pf.style.width='100%';pp.innerText='100%';ps.innerText='Selesai!';"
"        tt.className='toast ok';tt.innerText='SUKSES: Firmware ESP32 berhasil di-flash! ESP32 akan reboot otomatis.';tt.style.display='block';"
"      }"
"    }else{tt.className='toast err';tt.innerText='GAGAL: '+x.responseText;tt.style.display='block';b.disabled=false;}"
"  };"
"  x.onerror=function(){tt.className='toast err';tt.innerText='GAGAL: Terjadi gangguan jaringan saat mengunggah!';tt.style.display='block';b.disabled=false;};"
"  x.send(f);"
"}"
"function exitSta(){if(confirm('Kembali ke mode Wi-Fi Client (Station) dan menutup portal Web Dashboard?')){fetch('/api/mode',{method:'POST',body:'mode=sta'}).then(function(){alert('Portal ditutup. Mesin kembali ke mode normal.');}).catch(function(){alert('Perintah terkirim.');});}}"
"window.onload=function(){"
"  var d=new Date();var ym=d.getFullYear()+('0'+(d.getMonth()+1)).slice(-2)+('0'+d.getDate()).slice(-2);"
"  var di=document.getElementById('log-date-inp');if(di)di.value=ym;"
"  fetchStats();"
"};"
"</script>"
"</body>"
"</html>";


/* ========================================================================== */
/*                           HTTP SERVER HANDLERS                             */
/* ========================================================================== */

static esp_err_t ota_web_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, ota_html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t ota_update_stm32_post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Receiving STM32 Firmware upload (Size: %d bytes)...", (int)req->content_len);
    if (req->content_len <= 0 || req->content_len > 1024 * 1024) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Ukuran file tidak valid (Maksimal 1 MB).");
        return ESP_FAIL;
    }

    if (s_flash_state == FLASH_STATE_ERASING || s_flash_state == FLASH_STATE_PROGRAMMING) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Proses flashing lain sedang berlangsung.");
        return ESP_FAIL;
    }

    const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
    if (!part) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Partisi staging flash ota_1 tidak ditemukan!");
        return ESP_FAIL;
    }

    /* Erase partisi staging secukupnya */
    size_t erase_size = (req->content_len + 4095) & (~4095);
    if (esp_partition_erase_range(part, 0, erase_size) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Gagal mengosongkan partisi staging.");
        return ESP_FAIL;
    }

    s_flash_state = FLASH_STATE_UPLOADING;
    s_flash_progress = 0;
    snprintf(s_flash_status_msg, sizeof(s_flash_status_msg), "Mengunggah firmware ke gateway...");

    char rx_buf[1024];
    size_t total_size = req->content_len;
    size_t received_size = 0;

    while (received_size < total_size) {
        size_t to_recv = sizeof(rx_buf);
        if (to_recv > (total_size - received_size)) {
            to_recv = total_size - received_size;
        }

        int ret = httpd_req_recv(req, rx_buf, to_recv);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            ESP_LOGE(TAG, "Upload receive failed: %d", ret);
            s_flash_state = FLASH_STATE_ERROR;
            snprintf(s_flash_status_msg, sizeof(s_flash_status_msg), "Koneksi terputus saat mengunggah file.");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Koneksi terputus saat mengunggah file.");
            return ESP_FAIL;
        }

        if (esp_partition_write(part, received_size, rx_buf, ret) != ESP_OK) {
            ESP_LOGE(TAG, "Failed writing to staging partition!");
            s_flash_state = FLASH_STATE_ERROR;
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Gagal menulis ke partisi flash staging.");
            return ESP_FAIL;
        }

        received_size += ret;
    }

    s_staged_firmware_size = received_size;
    s_flash_state = FLASH_STATE_READY;
    s_flash_progress = 5;
    snprintf(s_flash_status_msg, sizeof(s_flash_status_msg), "File firmware diterima (%u KB). Memulai flash STM32...", (unsigned int)(s_staged_firmware_size / 1024));

    /* Luncurkan task flashing di background */
    xTaskCreate(stm32_flasher_task, "stm32_flash", 4096, NULL, 5, NULL);

    const char *resp = "{\"success\":true,\"message\":\"File firmware berhasil diunggah. Flashing STM32 sedang berjalan di background.\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static void delayed_restart_task(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(1800));
    esp_restart();
}

static esp_err_t ota_update_esp32_post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Starting ESP32 Native OTA Update (Size: %d bytes)...", (int)req->content_len);
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Partisi OTA tidak ditemukan pada layout flash!");
        return ESP_FAIL;
    }

    esp_ota_handle_t update_handle = 0;
    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Gagal menginisialisasi partisi OTA ESP32.");
        return ESP_FAIL;
    }

    char ota_write_data[1024];
    size_t total_size = req->content_len;
    size_t received_size = 0;
    int last_pct = -1;

    while (received_size < total_size) {
        size_t to_recv = sizeof(ota_write_data);
        if (to_recv > (total_size - received_size)) {
            to_recv = total_size - received_size;
        }

        int ret = httpd_req_recv(req, ota_write_data, to_recv);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            ESP_LOGE(TAG, "HTTP receive failed: %d", ret);
            esp_ota_abort(update_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Koneksi terputus saat mengunduh data firmware ESP32.");
            return ESP_FAIL;
        }

        err = esp_ota_write(update_handle, (const void *)ota_write_data, ret);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
            esp_ota_abort(update_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Gagal menulis blok partisi OTA ESP32!");
            return ESP_FAIL;
        }

        received_size += ret;
        int pct = (int)((received_size * 100) / total_size);
        if (pct != last_pct && (pct % 5 == 0 || pct == 100)) {
            last_pct = pct;
            char uart_buf[64];
            snprintf(uart_buf, sizeof(uart_buf), "OTA_PROGRESS:%d|Mengisi partisi ESP32...\n", pct);
            uart_send_and_log(uart_buf);
        }
    }

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed (%s)", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Validasi image OTA ESP32 gagal.");
        return ESP_FAIL;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Gagal mengalihkan partisi boot aktif.");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "ESP32 OTA Update complete! Rebooting soon...");
    uart_send_and_log("OTA_PROGRESS:100|Pembaruan ESP32 sukses! Rebooting...\n");

    const char *resp = "{\"success\":true,\"message\":\"Firmware ESP32 berhasil di-update! Sistem akan reboot dalam 2 detik.\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    xTaskCreate(delayed_restart_task, "delayed_restart", 2048, NULL, 5, NULL);
    return ESP_OK;
}

static esp_err_t ota_api_flash_status_get_handler(httpd_req_t *req) {
    char json[256];
    const char *st_str = "idle";
    if (s_flash_state == FLASH_STATE_UPLOADING) st_str = "uploading";
    else if (s_flash_state == FLASH_STATE_READY) st_str = "ready";
    else if (s_flash_state == FLASH_STATE_ERASING) st_str = "erasing";
    else if (s_flash_state == FLASH_STATE_PROGRAMMING) st_str = "programming";
    else if (s_flash_state == FLASH_STATE_SUCCESS) st_str = "success";
    else if (s_flash_state == FLASH_STATE_ERROR) st_str = "error";

    snprintf(json, sizeof(json),
             "{\"state\":\"%s\",\"progress\":%d,\"message\":\"%s\"}",
             st_str, s_flash_progress, s_flash_status_msg);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t dashboard_api_stats_get_handler(httpd_req_t *req) {
    if (s_stm_stats_sem) {
        xSemaphoreTake(s_stm_stats_sem, 0);
    }
    uart_send_and_log("CMD:GET_STM_STATS\n");
    if (s_stm_stats_sem) {
        xSemaphoreTake(s_stm_stats_sem, pdMS_TO_TICKS(500));
    }

    char json[512];
    snprintf(json, sizeof(json),
        "{\"esp32\":{\"temp\":%.1f,\"free_heap\":%lu,\"uptime\":%lu,\"mode\":\"%s\",\"wifi_status\":\"%s\",\"wifi_ssid\":\"%s\",\"wifi_rssi\":\"%s\"},"
        "\"stm32\":{\"temp\":%.1f,\"rfid_ready\":%s,\"rfid_version\":\"%s\",\"sd_mounted\":%s,\"pending_count\":%lu,\"uptime\":%lu}}",
        get_esp32_temperature(),
        (unsigned long)esp_get_free_heap_size(),
        (unsigned long)(esp_timer_get_time() / 1000000),
        s_is_ap_mode ? "SoftAP" : "Station",
        (xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT) ? "Connected" : "Disconnected",
        s_wifi_ssid_buf, s_wifi_rssi_buf,
        s_stm32_stats.temp,
        s_stm32_stats.rfid_ready ? "true" : "false",
        s_stm32_stats.rfid_ver[0] ? s_stm32_stats.rfid_ver : "0x18",
        s_stm32_stats.sd_mounted ? "true" : "false",
        (unsigned long)s_stm32_stats.pending_count,
        (unsigned long)s_stm32_stats.uptime_sec);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t dashboard_api_settings_get_handler(httpd_req_t *req) {
    if (s_stm_config_sem) {
        xSemaphoreTake(s_stm_config_sem, 0);
    }
    uart_send_and_log("CMD:GET_ALL_CONFIG\n");
    if (s_stm_config_sem) {
        xSemaphoreTake(s_stm_config_sem, pdMS_TO_TICKS(600));
    }

    char saved_ssid[34] = "";
    char saved_pass[66] = "";
    nvs_handle_t my_handle;
    if (nvs_open("storage", NVS_READONLY, &my_handle) == ESP_OK) {
        size_t req_len = sizeof(saved_ssid);
        nvs_get_str(my_handle, "ssid", saved_ssid, &req_len);
        req_len = sizeof(saved_pass);
        nvs_get_str(my_handle, "pass", saved_pass, &req_len);
        nvs_close(my_handle);
    }

    char code[32] = "GATE-001", name[32] = "Gate Utama", pin[16] = "123456";
    int sync = 0, sync_int = 15, apb = 60, bright = 80, sleep = 30, theme = 0, rot = 0, lpcd_d = 3, lpcd_i = 300;

    if (strlen(s_stm_config_str) > 0) {
        char buf[256];
        strncpy(buf, s_stm_config_str, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        char *tok = strtok(buf, "|");
        while (tok) {
            if (strncmp(tok, "DEV_CODE=", 9) == 0) strncpy(code, tok + 9, sizeof(code) - 1);
            else if (strncmp(tok, "DEV_NAME=", 9) == 0) strncpy(name, tok + 9, sizeof(name) - 1);
            else if (strncmp(tok, "PIN=", 4) == 0) strncpy(pin, tok + 4, sizeof(pin) - 1);
            else if (strncmp(tok, "SYNC=", 5) == 0) sync = atoi(tok + 5);
            else if (strncmp(tok, "INT=", 4) == 0) sync_int = atoi(tok + 4);
            else if (strncmp(tok, "APB=", 4) == 0) apb = atoi(tok + 4);
            else if (strncmp(tok, "BRIGHT=", 7) == 0) bright = atoi(tok + 7);
            else if (strncmp(tok, "SLEEP=", 6) == 0) sleep = atoi(tok + 6);
            else if (strncmp(tok, "THEME=", 6) == 0) theme = atoi(tok + 6);
            else if (strncmp(tok, "ROT=", 4) == 0) rot = atoi(tok + 4);
            else if (strncmp(tok, "LPCD_D=", 7) == 0) lpcd_d = atoi(tok + 7);
            else if (strncmp(tok, "LPCD_I=", 7) == 0) lpcd_i = atoi(tok + 7);
            tok = strtok(NULL, "|");
        }
    }

    char json[512];
    snprintf(json, sizeof(json),
        "{\"dev_code\":\"%s\",\"dev_name\":\"%s\",\"admin_pin\":\"%s\","
        "\"wifi_ssid\":\"%s\",\"wifi_pass\":\"%s\","
        "\"sync_mode\":%d,\"sync_interval\":%d,\"antipassback\":%d,"
        "\"brightness\":%d,\"sleep_timeout\":%d,\"theme\":%d,\"rotation\":%d,"
        "\"lpcd_delta\":%d,\"lpcd_interval\":%d}",
        code, name, pin, saved_ssid, saved_pass,
        sync, sync_int, apb, bright, sleep, theme, rot, lpcd_d, lpcd_i);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t dashboard_api_settings_post_handler(httpd_req_t *req) {
    char rx_buf[600] = {0};
    int ret = httpd_req_recv(req, rx_buf, sizeof(rx_buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload kosong.");
        return ESP_FAIL;
    }
    rx_buf[ret] = '\0';

    cJSON *root = cJSON_Parse(rx_buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Format JSON tidak valid.");
        return ESP_FAIL;
    }

    cJSON *c_code = cJSON_GetObjectItem(root, "dev_code");
    cJSON *c_name = cJSON_GetObjectItem(root, "dev_name");
    cJSON *c_pin  = cJSON_GetObjectItem(root, "admin_pin");
    cJSON *c_ssid = cJSON_GetObjectItem(root, "wifi_ssid");
    cJSON *c_pass = cJSON_GetObjectItem(root, "wifi_pass");
    cJSON *c_sync = cJSON_GetObjectItem(root, "sync_mode");
    cJSON *c_int  = cJSON_GetObjectItem(root, "sync_interval");
    cJSON *c_apb  = cJSON_GetObjectItem(root, "antipassback");
    cJSON *c_brt  = cJSON_GetObjectItem(root, "brightness");
    cJSON *c_slp  = cJSON_GetObjectItem(root, "sleep_timeout");
    cJSON *c_thm  = cJSON_GetObjectItem(root, "theme");
    cJSON *c_rot  = cJSON_GetObjectItem(root, "rotation");
    cJSON *c_lp_d = cJSON_GetObjectItem(root, "lpcd_delta");
    cJSON *c_lp_i = cJSON_GetObjectItem(root, "lpcd_interval");

    if (c_ssid && cJSON_IsString(c_ssid) && strlen(c_ssid->valuestring) > 0) {
        nvs_handle_t my_handle;
        if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
            nvs_set_str(my_handle, "ssid", c_ssid->valuestring);
            if (c_pass && cJSON_IsString(c_pass)) {
                nvs_set_str(my_handle, "pass", c_pass->valuestring);
            }
            nvs_commit(my_handle);
            nvs_close(my_handle);
        }
    }

    char cmd[300];
    snprintf(cmd, sizeof(cmd),
        "CMD:SET_ALL_CONFIG:DEV_CODE=%s|DEV_NAME=%s|PIN=%s|SYNC=%d|INT=%d|APB=%d|BRIGHT=%d|SLEEP=%d|THEME=%d|ROT=%d|LPCD_D=%d|LPCD_I=%d\n",
        (c_code && cJSON_IsString(c_code)) ? c_code->valuestring : s_device_code,
        (c_name && cJSON_IsString(c_name)) ? c_name->valuestring : s_device_name,
        (c_pin && cJSON_IsString(c_pin)) ? c_pin->valuestring : "123456",
        (c_sync && cJSON_IsNumber(c_sync)) ? c_sync->valueint : 0,
        (c_int && cJSON_IsNumber(c_int)) ? c_int->valueint : 15,
        (c_apb && cJSON_IsNumber(c_apb)) ? c_apb->valueint : 60,
        (c_brt && cJSON_IsNumber(c_brt)) ? c_brt->valueint : 80,
        (c_slp && cJSON_IsNumber(c_slp)) ? c_slp->valueint : 30,
        (c_thm && cJSON_IsNumber(c_thm)) ? c_thm->valueint : 0,
        (c_rot && cJSON_IsNumber(c_rot)) ? c_rot->valueint : 0,
        (c_lp_d && cJSON_IsNumber(c_lp_d)) ? c_lp_d->valueint : 3,
        (c_lp_i && cJSON_IsNumber(c_lp_i)) ? c_lp_i->valueint : 300);

    cJSON_Delete(root);

    if (s_stm_config_saved_sem) {
        xSemaphoreTake(s_stm_config_saved_sem, 0);
    }
    uart_send_and_log(cmd);
    if (s_stm_config_saved_sem) {
        xSemaphoreTake(s_stm_config_saved_sem, pdMS_TO_TICKS(1000));
    }

    const char *resp = "{\"success\":true,\"message\":\"Semua pengaturan berhasil disimpan ke Flash Sector 11!\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t dashboard_api_log_preview_get_handler(httpd_req_t *req) {
    char date_param[32] = "";
    char query[64];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        httpd_query_key_value(query, "date", date_param, sizeof(date_param));
    }

    char req_fn[48];
    if (strlen(date_param) >= 8) {
        snprintf(req_fn, sizeof(req_fn), "ATT_%.8s.CSV", date_param);
    } else {
        strcpy(req_fn, "");
    }

    char uart_cmd[64];
    snprintf(uart_cmd, sizeof(uart_cmd), "CMD:READ_LOG:%s\n", req_fn);

    if (s_log_stream_sem) {
        xSemaphoreTake(s_log_stream_sem, 0);
    }
    s_log_stream.count = 0;
    uart_send_and_log(uart_cmd);
    if (s_log_stream_sem) {
        xSemaphoreTake(s_log_stream_sem, pdMS_TO_TICKS(2500));
    }

    cJSON *resp_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(resp_obj, "filename", s_log_stream.filename[0] ? s_log_stream.filename : (req_fn[0] ? req_fn : "ATTEND.CSV"));
    cJSON_AddNumberToObject(resp_obj, "count", s_log_stream.count);
    cJSON *lines_arr = cJSON_CreateArray();
    for (int i = 0; i < s_log_stream.count; i++) {
        cJSON_AddItemToArray(lines_arr, cJSON_CreateString(s_log_stream.lines[i]));
    }
    cJSON_AddItemToObject(resp_obj, "lines", lines_arr);

    char *json_out = cJSON_PrintUnformatted(resp_obj);
    cJSON_Delete(resp_obj);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_out, HTTPD_RESP_USE_STRLEN);
    free(json_out);
    return ESP_OK;
}

static esp_err_t dashboard_api_log_download_get_handler(httpd_req_t *req) {
    char date_param[32] = "";
    char query[64];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        httpd_query_key_value(query, "date", date_param, sizeof(date_param));
    }

    char req_fn[48];
    if (strlen(date_param) >= 8) {
        snprintf(req_fn, sizeof(req_fn), "ATT_%.8s.CSV", date_param);
    } else {
        strcpy(req_fn, "");
    }

    char uart_cmd[64];
    snprintf(uart_cmd, sizeof(uart_cmd), "CMD:READ_LOG:%s\n", req_fn);

    if (s_log_stream_sem) {
        xSemaphoreTake(s_log_stream_sem, 0);
    }
    s_log_stream.count = 0;
    uart_send_and_log(uart_cmd);
    if (s_log_stream_sem) {
        xSemaphoreTake(s_log_stream_sem, pdMS_TO_TICKS(2500));
    }

    const char *out_name = s_log_stream.filename[0] ? s_log_stream.filename : (req_fn[0] ? req_fn : "ATTEND.CSV");
    char disp_hdr[96];
    snprintf(disp_hdr, sizeof(disp_hdr), "attachment; filename=\"%s\"", out_name);

    httpd_resp_set_type(req, "text/csv; charset=utf-8");
    httpd_resp_set_hdr(req, "Content-Disposition", disp_hdr);

    for (int i = 0; i < s_log_stream.count; i++) {
        char line_out[160];
        snprintf(line_out, sizeof(line_out), "%s\r\n", s_log_stream.lines[i]);
        httpd_resp_send_chunk(req, line_out, strlen(line_out));
    }
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t dashboard_api_users_preview_get_handler(httpd_req_t *req) {
    if (s_user_stream_sem) {
        xSemaphoreTake(s_user_stream_sem, 0);
    }
    s_user_stream.count = 0;
    uart_send_and_log("CMD:READ_USERS\n");
    if (s_user_stream_sem) {
        xSemaphoreTake(s_user_stream_sem, pdMS_TO_TICKS(2500));
    }

    cJSON *resp_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp_obj, "count", s_user_stream.count);
    cJSON *lines_arr = cJSON_CreateArray();
    for (int i = 0; i < s_user_stream.count; i++) {
        cJSON_AddItemToArray(lines_arr, cJSON_CreateString(s_user_stream.lines[i]));
    }
    cJSON_AddItemToObject(resp_obj, "lines", lines_arr);

    char *json_out = cJSON_PrintUnformatted(resp_obj);
    cJSON_Delete(resp_obj);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_out, HTTPD_RESP_USE_STRLEN);
    free(json_out);
    return ESP_OK;
}

static esp_err_t dashboard_api_users_download_get_handler(httpd_req_t *req) {
    if (s_user_stream_sem) {
        xSemaphoreTake(s_user_stream_sem, 0);
    }
    s_user_stream.count = 0;
    uart_send_and_log("CMD:READ_USERS\n");
    if (s_user_stream_sem) {
        xSemaphoreTake(s_user_stream_sem, pdMS_TO_TICKS(2500));
    }

    httpd_resp_set_type(req, "text/csv; charset=utf-8");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"USERS.CSV\"");

    for (int i = 0; i < s_user_stream.count; i++) {
        char line_out[160];
        snprintf(line_out, sizeof(line_out), "%s\r\n", s_user_stream.lines[i]);
        httpd_resp_send_chunk(req, line_out, strlen(line_out));
    }
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t dashboard_api_users_upload_post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Uploading USERS.CSV (size: %d)...", (int)req->content_len);
    if (req->content_len <= 0 || req->content_len > 64 * 1024) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Ukuran file tidak valid (Maks 64 KB).");
        return ESP_FAIL;
    }

    if (s_user_write_sem) {
        xSemaphoreTake(s_user_write_sem, 0);
    }
    uart_send_and_log("CMD:USER_WRITE_START\n");
    if (s_user_write_sem) {
        if (xSemaphoreTake(s_user_write_sem, pdMS_TO_TICKS(1500)) != pdTRUE || !s_user_write_ok) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "MicroSD gagal membuka file USERS.TMP.");
            return ESP_FAIL;
        }
    }

    char rx_buf[1024];
    size_t total_recv = 0;
    char line_acc[256];
    size_t line_acc_len = 0;

    while (total_recv < req->content_len) {
        size_t to_recv = sizeof(rx_buf) - 1;
        if (to_recv > (req->content_len - total_recv)) {
            to_recv = req->content_len - total_recv;
        }

        int ret = httpd_req_recv(req, rx_buf, to_recv);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) continue;
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Koneksi terputus saat upload.");
            return ESP_FAIL;
        }
        total_recv += ret;

        for (int i = 0; i < ret; i++) {
            char c = rx_buf[i];
            if (c == '\n' || c == '\r') {
                if (line_acc_len > 0) {
                    line_acc[line_acc_len] = '\0';
                    char uart_cmd[300];
                    snprintf(uart_cmd, sizeof(uart_cmd), "CMD:USER_LINE:%s\n", line_acc);
                    uart_send_and_log(uart_cmd);
                    vTaskDelay(pdMS_TO_TICKS(5));
                    line_acc_len = 0;
                }
            } else {
                if (line_acc_len < sizeof(line_acc) - 1) {
                    line_acc[line_acc_len++] = c;
                }
            }
        }
    }

    if (line_acc_len > 0) {
        line_acc[line_acc_len] = '\0';
        char uart_cmd[300];
        snprintf(uart_cmd, sizeof(uart_cmd), "CMD:USER_LINE:%s\n", line_acc);
        uart_send_and_log(uart_cmd);
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    if (s_user_write_sem) {
        xSemaphoreTake(s_user_write_sem, 0);
    }
    uart_send_and_log("CMD:USER_WRITE_END\n");
    if (s_user_write_sem) {
        xSemaphoreTake(s_user_write_sem, pdMS_TO_TICKS(2000));
    }

    const char *resp = "{\"success\":true,\"message\":\"File USERS.CSV berhasil diperbarui di MicroSD!\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t ota_api_status_get_handler(httpd_req_t *req) {
    char status_json[256];
    snprintf(status_json, sizeof(status_json),
             "{\"device_code\":\"%s\",\"device_name\":\"%s\",\"mode\":\"%s\",\"free_heap\":%lu}",
             s_device_code, s_device_name, s_is_ap_mode ? "SoftAP" : "Station",
             (unsigned long)esp_get_free_heap_size());
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, status_json, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static void wifi_switch_to_sta(void);

static esp_err_t ota_api_mode_post_handler(httpd_req_t *req) {
    const char *resp = "{\"status\":\"ok\",\"message\":\"Beralih ke mode Client (Station)...\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    wifi_switch_to_sta();
    return ESP_OK;
}

static void start_web_server(void) {
    if (s_http_server) return;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.ctrl_port = 32768;
    config.max_open_sockets = 7;
    config.max_uri_handlers = 20;
    config.stack_size = 10240;
    config.lru_purge_enable = true;
    config.recv_wait_timeout = 30; // 30 detik
    config.send_wait_timeout = 30; // 30 detik

    httpd_uri_t uri_get_index = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = ota_web_get_handler,
        .user_ctx = NULL
    };
    httpd_uri_t uri_get_stats = {
        .uri = "/api/stats",
        .method = HTTP_GET,
        .handler = dashboard_api_stats_get_handler,
        .user_ctx = NULL
    };
    httpd_uri_t uri_get_settings = {
        .uri = "/api/settings",
        .method = HTTP_GET,
        .handler = dashboard_api_settings_get_handler,
        .user_ctx = NULL
    };
    httpd_uri_t uri_post_settings = {
        .uri = "/api/settings",
        .method = HTTP_POST,
        .handler = dashboard_api_settings_post_handler,
        .user_ctx = NULL
    };
    httpd_uri_t uri_get_log_preview = {
        .uri = "/api/log/preview",
        .method = HTTP_GET,
        .handler = dashboard_api_log_preview_get_handler,
        .user_ctx = NULL
    };
    httpd_uri_t uri_get_log_download = {
        .uri = "/api/log/download",
        .method = HTTP_GET,
        .handler = dashboard_api_log_download_get_handler,
        .user_ctx = NULL
    };
    httpd_uri_t uri_get_users_preview = {
        .uri = "/api/users/preview",
        .method = HTTP_GET,
        .handler = dashboard_api_users_preview_get_handler,
        .user_ctx = NULL
    };
    httpd_uri_t uri_get_users_download = {
        .uri = "/api/users/download",
        .method = HTTP_GET,
        .handler = dashboard_api_users_download_get_handler,
        .user_ctx = NULL
    };
    httpd_uri_t uri_post_users_upload = {
        .uri = "/api/users/upload",
        .method = HTTP_POST,
        .handler = dashboard_api_users_upload_post_handler,
        .user_ctx = NULL
    };
    httpd_uri_t uri_post_update_stm32 = {
        .uri = "/update/stm32",
        .method = HTTP_POST,
        .handler = ota_update_stm32_post_handler,
        .user_ctx = NULL
    };
    httpd_uri_t uri_post_update_esp32 = {
        .uri = "/update/esp32",
        .method = HTTP_POST,
        .handler = ota_update_esp32_post_handler,
        .user_ctx = NULL
    };
    httpd_uri_t uri_get_flash_status = {
        .uri = "/api/flash_status",
        .method = HTTP_GET,
        .handler = ota_api_flash_status_get_handler,
        .user_ctx = NULL
    };
    httpd_uri_t uri_get_status = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = ota_api_status_get_handler,
        .user_ctx = NULL
    };
    httpd_uri_t uri_post_mode = {
        .uri = "/api/mode",
        .method = HTTP_POST,
        .handler = ota_api_mode_post_handler,
        .user_ctx = NULL
    };

    if (httpd_start(&s_http_server, &config) == ESP_OK) {
        httpd_register_uri_handler(s_http_server, &uri_get_index);
        httpd_register_uri_handler(s_http_server, &uri_get_stats);
        httpd_register_uri_handler(s_http_server, &uri_get_settings);
        httpd_register_uri_handler(s_http_server, &uri_post_settings);
        httpd_register_uri_handler(s_http_server, &uri_get_log_preview);
        httpd_register_uri_handler(s_http_server, &uri_get_log_download);
        httpd_register_uri_handler(s_http_server, &uri_get_users_preview);
        httpd_register_uri_handler(s_http_server, &uri_get_users_download);
        httpd_register_uri_handler(s_http_server, &uri_post_users_upload);
        httpd_register_uri_handler(s_http_server, &uri_post_update_stm32);
        httpd_register_uri_handler(s_http_server, &uri_post_update_esp32);
        httpd_register_uri_handler(s_http_server, &uri_get_flash_status);
        httpd_register_uri_handler(s_http_server, &uri_get_status);
        httpd_register_uri_handler(s_http_server, &uri_post_mode);
        ESP_LOGI(TAG, "Web Dashboard HTTP Server started on port 80!");
    } else {
        ESP_LOGE(TAG, "Failed to start Web Dashboard HTTP Server!");
    }
}

static void stop_web_server(void) {
    if (s_http_server) {
        httpd_stop(s_http_server);
        s_http_server = NULL;
        ESP_LOGI(TAG, "OTA Web Server stopped.");
    }
}


static void wifi_switch_to_ap(void) {
    if (s_is_ap_mode) return;

    ESP_LOGI(TAG, "Switching to SoftAP mode for OTA Web Server...");
    esp_wifi_stop();

    if (!s_ap_netif) {
        s_ap_netif = esp_netif_create_default_wifi_ap();
    }

    wifi_config_t ap_config = {
        .ap = {
            .ssid_len = 0,
            .channel = 1,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .ssid_hidden = 0,
            .max_connection = 4,
            .beacon_interval = 100,
        },
    };
    snprintf((char *)ap_config.ap.ssid, sizeof(ap_config.ap.ssid), "ABSENSI-OTA-%.19s", s_device_code);
    strncpy((char *)ap_config.ap.password, "12345678", sizeof(ap_config.ap.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    s_is_ap_mode = true;
    set_led_color(16, 8, 0); /* Amber LED tanda SoftAP aktif */

    start_web_server();

    /* Kirim balasan aktif ke STM32 via UART */
    char reply[128];
    snprintf(reply, sizeof(reply), "AP_ACTIVE:%s|12345678|192.168.4.1\n", ap_config.ap.ssid);
    uart_send_and_log(reply);
}

static void wifi_switch_to_sta(void) {
    if (!s_is_ap_mode) return;

    ESP_LOGI(TAG, "Switching back to Station (Client) mode...");
    stop_web_server();

    esp_wifi_stop();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    s_is_ap_mode = false;
    set_led_color(0, 0, 8);

    uart_send_and_log("STA_ACTIVE\n");
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

    /* Jika ESP32 sedang mem-flash STM32 via bootloader, jangan baca UART di sini */
    if (s_flashing_stm32_active) {
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

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
            } else if (strncmp(rfid_str, "CMD:START_AP", 12) == 0) {
              wifi_switch_to_ap();
            } else if (strncmp(rfid_str, "CMD:START_STA", 13) == 0) {
              wifi_switch_to_sta();
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
            } else if (strncmp(rfid_str, "STM_STATS:", 10) == 0) {
              /* Format: STM_STATS:TEMP=%.1f|RFID=%d,0x%02X|SD=%d|PEND=%lu|UP=%lu */
              char *p = rfid_str + 10;
              float temp = 0.0f;
              int rfid = 0, sd = 0;
              unsigned int rfid_v = 0;
              unsigned long pend = 0, up = 0;
              if (sscanf(p, "TEMP=%f|RFID=%d,0x%X|SD=%d|PEND=%lu|UP=%lu",
                         &temp, &rfid, &rfid_v, &sd, &pend, &up) >= 4) {
                s_stm32_stats.temp = temp;
                s_stm32_stats.rfid_ready = rfid;
                snprintf(s_stm32_stats.rfid_ver, sizeof(s_stm32_stats.rfid_ver), "0x%02X", rfid_v);
                s_stm32_stats.sd_mounted = sd;
                s_stm32_stats.pending_count = (uint32_t)pend;
                s_stm32_stats.uptime_sec = (uint32_t)up;
              }
              if (s_stm_stats_sem) xSemaphoreGive(s_stm_stats_sem);
            } else if (strncmp(rfid_str, "STM_CONFIG:", 11) == 0) {
              strncpy(s_stm_config_str, rfid_str + 11, sizeof(s_stm_config_str) - 1);
              s_stm_config_str[sizeof(s_stm_config_str) - 1] = '\0';
              if (s_stm_config_sem) xSemaphoreGive(s_stm_config_sem);
            } else if (strncmp(rfid_str, "CONFIG_SAVED", 12) == 0) {
              if (s_stm_config_saved_sem) xSemaphoreGive(s_stm_config_saved_sem);
            } else if (strncmp(rfid_str, "LOG_START:", 10) == 0) {
              s_log_stream.count = 0;
              s_log_stream.in_progress = true;
              s_log_stream.is_eof = false;
              char *fn = rfid_str + 10;
              char *nl = strpbrk(fn, "\r\n");
              if (nl) *nl = '\0';
              strncpy(s_log_stream.filename, fn, sizeof(s_log_stream.filename) - 1);
            } else if (strncmp(rfid_str, "LOG_LINE:", 9) == 0) {
              if (s_log_stream.in_progress && s_log_stream.count < MAX_WEB_LINES) {
                char *l = rfid_str + 9;
                char *nl = strpbrk(l, "\r\n");
                if (nl) *nl = '\0';
                strncpy(s_log_stream.lines[s_log_stream.count], l, MAX_LINE_LEN - 1);
                s_log_stream.lines[s_log_stream.count][MAX_LINE_LEN - 1] = '\0';
                s_log_stream.count++;
              }
            } else if (strncmp(rfid_str, "LOG_EOF", 7) == 0 || strncmp(rfid_str, "LOG_ERR:", 8) == 0) {
              s_log_stream.in_progress = false;
              s_log_stream.is_eof = true;
              if (s_log_stream_sem) xSemaphoreGive(s_log_stream_sem);
            } else if (strncmp(rfid_str, "USER_START", 10) == 0) {
              s_user_stream.count = 0;
              s_user_stream.in_progress = true;
              s_user_stream.is_eof = false;
            } else if (strncmp(rfid_str, "USER_LINE:", 10) == 0) {
              if (s_user_stream.in_progress && s_user_stream.count < MAX_WEB_LINES) {
                char *l = rfid_str + 10;
                char *nl = strpbrk(l, "\r\n");
                if (nl) *nl = '\0';
                strncpy(s_user_stream.lines[s_user_stream.count], l, MAX_LINE_LEN - 1);
                s_user_stream.lines[s_user_stream.count][MAX_LINE_LEN - 1] = '\0';
                s_user_stream.count++;
              }
            } else if (strncmp(rfid_str, "USER_EOF", 8) == 0 || strncmp(rfid_str, "USER_ERR:", 9) == 0) {
              s_user_stream.in_progress = false;
              s_user_stream.is_eof = true;
              if (s_user_stream_sem) xSemaphoreGive(s_user_stream_sem);
            } else if (strncmp(rfid_str, "USER_WRITE_READY", 16) == 0) {
              s_user_write_ok = true;
              if (s_user_write_sem) xSemaphoreGive(s_user_write_sem);
            } else if (strncmp(rfid_str, "USER_WRITE_OK", 13) == 0) {
              s_user_write_ok = true;
              if (s_user_write_sem) xSemaphoreGive(s_user_write_sem);
            } else if (strncmp(rfid_str, "USER_WRITE_FAIL", 15) == 0) {
              s_user_write_ok = false;
              if (s_user_write_sem) xSemaphoreGive(s_user_write_sem);
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

  // Initialize Web Dashboard Semaphores
  s_stm_stats_sem = xSemaphoreCreateBinary();
  s_stm_config_sem = xSemaphoreCreateBinary();
  s_stm_config_saved_sem = xSemaphoreCreateBinary();
  s_log_stream_sem = xSemaphoreCreateBinary();
  s_user_stream_sem = xSemaphoreCreateBinary();
  s_user_write_sem = xSemaphoreCreateBinary();

  // Initialize ESP32-C6 On-Chip Temperature Sensor
  init_esp32_temp_sensor();

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

  // Initialize STM32 Bootloader & Reset Control Pins (GPIO 21 BOOT0, GPIO 22 NRST)
  stm32_gpio_init();

  // Initialize WiFi first so event group is created
  wifi_init_sta();

  // Create Tasks
  xTaskCreate(button_task, "button_task", 4096, NULL, 5, NULL);
  xTaskCreate(uart_task, "uart_task", 4096, NULL, 5, NULL);
  xTaskCreate(sync_time_task, "sync_time", 4096, NULL, 4, NULL);
}

