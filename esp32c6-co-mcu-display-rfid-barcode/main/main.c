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
"<title>Portal Update Firmware OTA - Absensi RFID</title>"
"<style>"
":root{--bg:#0f172a;--card:#1e293b;--border:#334155;--cyan:#06b6d4;--cyan-h:#0891b2;--amber:#f59e0b;--red:#ef4444;--green:#10b981;--text:#f8fafc;--muted:#94a3b8;}"
"*{box-sizing:border-box;margin:0;padding:0;font-family:-apple-system,BlinkMacSystemFont,\"Segoe UI\",Roboto,sans-serif;}"
"body{background:var(--bg);color:var(--text);padding:18px;min-height:100vh;}"
".wrap{max-width:800px;margin:0 auto;}"
".hdr{display:flex;justify-content:space-between;align-items:center;border-bottom:2px solid var(--border);padding-bottom:14px;margin-bottom:20px;flex-wrap:wrap;gap:10px;}"
".hdr h1{font-size:22px;font-weight:700;color:#fff;}"
".hdr p{font-size:12px;color:var(--muted);margin-top:3px;}"
".badge{background:#064e3b;color:#34d399;border:1px solid #059669;padding:5px 12px;border-radius:9999px;font-size:11px;font-weight:600;}"
".warn{background:rgba(245,158,11,0.12);border:1px solid var(--amber);border-radius:10px;padding:14px 18px;margin-bottom:20px;display:flex;gap:12px;align-items:flex-start;}"
".warn .ico{font-size:22px;line-height:1;color:var(--amber);}"
".warn h3{font-size:14px;color:#fde68a;font-weight:600;margin-bottom:3px;}"
".warn p{font-size:12px;color:#cbd5e1;line-height:1.4;}"
".guide{background:#131d2e;border:1px solid var(--border);border-radius:10px;padding:14px 18px;margin-bottom:20px;font-size:12px;color:#cbd5e1;line-height:1.6;}"
".guide h4{font-size:13px;color:var(--cyan);margin-bottom:8px;font-weight:600;}"
".guide ol{padding-left:18px;}"
".grid{display:grid;grid-template-columns:1fr;gap:18px;margin-bottom:20px;}"
"@media(min-width:768px){.grid{grid-template-columns:1fr 1fr;}}"
".card{background:var(--card);border:1px solid var(--border);border-radius:12px;padding:18px;display:flex;flex-direction:column;}"
".chdr{display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;}"
".chdr h2{font-size:16px;font-weight:600;color:#fff;}"
".chip{font-size:10px;padding:3px 8px;border-radius:6px;font-weight:600;}"
".chip-s{background:#1e3a8a;color:#93c5fd;}"
".chip-e{background:#3b0764;color:#d8b4fe;}"
".desc{font-size:12px;color:var(--muted);margin-bottom:14px;line-height:1.4;flex-grow:1;}"
".fbox{border:2px dashed var(--border);border-radius:8px;padding:14px;text-align:center;cursor:pointer;margin-bottom:14px;transition:0.2s;}"
".fbox:hover{border-color:var(--cyan);background:rgba(6,182,212,0.06);}"
".fbox input{display:none;}"
".fbox .ltxt{font-size:12px;color:var(--muted);}"
".btn{width:100%;padding:11px;border:none;border-radius:7px;font-size:13px;font-weight:600;cursor:pointer;transition:0.2s;}"
".btn-pri{background:var(--cyan);color:#082f49;}"
".btn-pri:hover:not(:disabled){background:var(--cyan-h);}"
".btn-pri:disabled{opacity:0.5;cursor:not-allowed;}"
".btn-ext{background:#334155;color:#f8fafc;margin-top:10px;}"
".btn-ext:hover{background:#475569;}"
".pwrap{margin-top:12px;display:none;}"
".pbar{background:#0f172a;height:10px;border-radius:9999px;overflow:hidden;}"
".pfill{background:linear-gradient(90deg,#06b6d4,#10b981);height:100%;width:0%;transition:width 0.25s;}"
".ptxt{display:flex;justify-content:space-between;font-size:11px;color:var(--muted);margin-top:5px;}"
".toast{margin-top:10px;padding:9px;border-radius:7px;font-size:11px;display:none;line-height:1.4;}"
".toast.ok{background:rgba(16,185,129,0.15);border:1px solid var(--green);color:#6ee7b7;}"
".toast.err{background:rgba(239,68,68,0.15);border:1px solid var(--red);color:#fca5a5;}"
"</style>"
"</head>"
"<body>"
"<div class=\"wrap\">"
"<div class=\"hdr\">"
"<div><h1>Portal Update Firmware OTA</h1><p>Mesin Absensi RFID Gateway &bull; SoftAP Mode (192.168.4.1)</p></div>"
"<span class=\"badge\">&#9679; SoftAP Siap</span>"
"</div>"
"<div class=\"warn\"><div class=\"ico\">&#9888;</div><div>"
"<h3>PERINGATAN OPERATOR LAPANGAN</h3>"
"<p>Jangan mematikan atau mencabut daya mesin saat proses flashing berlangsung! Tunggu hingga proses 100% dan status sukses ditampilkan.</p>"
"</div></div>"
"<div class=\"guide\"><h4>Petunjuk Penggunaan:</h4><ol>"
"<li>Siapkan file binary berekstensi <b>.bin</b> (firmware hasil build STM32 atau ESP32).</li>"
"<li>Pilih kotak file pada card yang sesuai, lalu tekan tombol <b>Mulai Update</b>.</li>"
"<li>Pantau bar progres di browser ini dan layar LCD mesin absensi.</li>"
"<li>Setelah selesai, tekan <b>Kembali ke Mode Normal (Client)</b> di bawah.</li>"
"</ol></div>"
"<div class=\"grid\">"
"<div class=\"card\">"
"<div class=\"chdr\"><h2>STM32 Main MCU</h2><span class=\"chip chip-s\">STM32F405</span></div>"
"<p class=\"desc\">Mengontrol UI Display LVGL 480x320, RFID Reader CLRC663, MicroSD FatFS, dan antrean data lokal. Update via ROM Bootloader UART.</p>"
"<div class=\"fbox\" onclick=\"document.getElementById('f-s').click()\">"
"<input type=\"file\" id=\"f-s\" accept=\".bin\" onchange=\"onF(this,'l-s','b-s')\">"
"<div class=\"ltxt\" id=\"l-s\">&#128190; Pilih File <b>firmware_stm32.bin</b></div>"
"</div>"
"<button class=\"btn btn-pri\" id=\"b-s\" disabled onclick=\"upF('stm32')\">&#9654; Mulai Update STM32</button>"
"<div class=\"pwrap\" id=\"pw-stm32\"><div class=\"pbar\"><div class=\"pfill\" id=\"pf-stm32\"></div></div>"
"<div class=\"ptxt\"><span id=\"ps-stm32\">Mengunggah...</span><span id=\"pp-stm32\">0%</span></div></div>"
"<div class=\"toast\" id=\"t-stm32\"></div>"
"</div>"
"<div class=\"card\">"
"<div class=\"chdr\"><h2>ESP32-C6 Co-MCU</h2><span class=\"chip chip-e\">ESP32-C6 Wi-Fi 6</span></div>"
"<p class=\"desc\">Mengontrol koneksi Wi-Fi 6/4, protokol sinkronisasi HTTP backend, dan Web Server OTA. Update via native dual-partition OTA.</p>"
"<div class=\"fbox\" onclick=\"document.getElementById('f-e').click()\">"
"<input type=\"file\" id=\"f-e\" accept=\".bin\" onchange=\"onF(this,'l-e','b-e')\">"
"<div class=\"ltxt\" id=\"l-e\">&#128190; Pilih File <b>firmware_esp32.bin</b></div>"
"</div>"
"<button class=\"btn btn-pri\" id=\"b-e\" disabled onclick=\"upF('esp32')\">&#9654; Mulai Update ESP32</button>"
"<div class=\"pwrap\" id=\"pw-esp32\"><div class=\"pbar\"><div class=\"pfill\" id=\"pf-esp32\"></div></div>"
"<div class=\"ptxt\"><span id=\"ps-esp32\">Mengunggah...</span><span id=\"pp-esp32\">0%</span></div></div>"
"<div class=\"toast\" id=\"t-esp32\"></div>"
"</div>"
"</div>"
"<button class=\"btn btn-ext\" onclick=\"exitSta()\">&#10005; Selesai & Kembali ke Mode Client (Station)</button>"
"</div>"
"<script>"
"function onF(i,l,b){if(i.files&&i.files[0]){var f=i.files[0];document.getElementById(l).innerHTML='<b>'+f.name+'</b> ('+(f.size/1024).toFixed(1)+' KB)';document.getElementById(b).disabled=false;}}"
"var pollTimer=null;"
"function pollSTM32(){"
"  fetch('/api/flash_status').then(function(r){return r.json();}).then(function(d){"
"    var pf=document.getElementById('pf-stm32'),pp=document.getElementById('pp-stm32'),ps=document.getElementById('ps-stm32'),tt=document.getElementById('t-stm32'),b=document.getElementById('b-s');"
"    if(d.progress!==undefined){pf.style.width=d.progress+'%';pp.innerText=d.progress+'%';}"
"    if(d.message){ps.innerText=d.message;}"
"    if(d.state==='success'){"
"      clearInterval(pollTimer);"
"      pf.style.width='100%';pp.innerText='100%';ps.innerText='Selesai!';"
"      tt.className='toast ok';tt.innerText=d.message;tt.style.display='block';"
"    }else if(d.state==='error'){"
"      clearInterval(pollTimer);"
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
"        pollTimer=setInterval(pollSTM32,600);"
"      }else{"
"        pf.style.width='100%';pp.innerText='100%';ps.innerText='Selesai!';"
"        tt.className='toast ok';tt.innerText='SUKSES: Firmware ESP32 berhasil di-flash! ESP32 akan reboot otomatis.';tt.style.display='block';"
"      }"
"    }else{tt.className='toast err';tt.innerText='GAGAL: '+x.responseText;tt.style.display='block';b.disabled=false;}"
"  };"
"  x.onerror=function(){tt.className='toast err';tt.innerText='GAGAL: Terjadi gangguan jaringan saat mengunggah!';tt.style.display='block';b.disabled=false;};"
"  x.send(f);"
"}"
"function exitSta(){if(confirm('Kembali ke mode Wi-Fi Client (Station) dan menutup portal OTA?')){fetch('/api/mode',{method:'POST',body:'mode=sta'}).then(function(){alert('Portal OTA ditutup. Mesin kembali ke mode normal.');}).catch(function(){alert('Perintah terkirim.');});}}"
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
        httpd_register_uri_handler(s_http_server, &uri_post_update_stm32);
        httpd_register_uri_handler(s_http_server, &uri_post_update_esp32);
        httpd_register_uri_handler(s_http_server, &uri_get_flash_status);
        httpd_register_uri_handler(s_http_server, &uri_get_status);
        httpd_register_uri_handler(s_http_server, &uri_post_mode);
        ESP_LOGI(TAG, "OTA Web Server started on port 80!");
    } else {
        ESP_LOGE(TAG, "Failed to start OTA Web Server!");
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

  // Initialize STM32 Bootloader & Reset Control Pins (GPIO 21 BOOT0, GPIO 22 NRST)
  stm32_gpio_init();

  // Initialize WiFi first so event group is created
  wifi_init_sta();

  // Create Tasks
  xTaskCreate(button_task, "button_task", 4096, NULL, 5, NULL);
  xTaskCreate(uart_task, "uart_task", 4096, NULL, 5, NULL);
  xTaskCreate(sync_time_task, "sync_time", 4096, NULL, 4, NULL);
}
