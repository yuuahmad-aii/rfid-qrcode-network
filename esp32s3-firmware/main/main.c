/* ST7789 LVGL Example
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include <inttypes.h>
#include "rc522.h"

#if __has_include("esp_lcd_panel_st7789.h")
#include "esp_lcd_panel_st7789.h"
#endif

static const char *TAG = "main";

#define PIN_NUM_LCD_RST    5
#define PIN_NUM_LCD_CS     6
#define PIN_NUM_LCD_DC     7
#define PIN_NUM_SCLK       8
#define PIN_NUM_MOSI       9
#define PIN_NUM_MISO       -1
#define PIN_NUM_BK_LIGHT   38

// RC522 pins (SPI3)
#define RC522_SPI_HOST SPI3_HOST
#define RC522_MISO     13
#define RC522_MOSI     11
#define RC522_SCLK     12
#define RC522_CS       10
#define RC522_RST      1

lv_obj_t *rfid_label = NULL;

static void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data) {
    rc522_event_data_t* data = (rc522_event_data_t*) event_data;
    switch(event_id) {
        case RC522_EVENT_TAG_SCANNED: {
            rc522_tag_t* tag = (rc522_tag_t*) data->ptr;
            ESP_LOGI(TAG, "Tag scanned (sn: %" PRIu64 ")", tag->serial_number);
            
            if (rfid_label != NULL) {
                lvgl_port_lock(0);
                lv_label_set_text_fmt(rfid_label, "UID:\n%" PRIu64, tag->serial_number);
                lv_obj_center(rfid_label);
                lvgl_port_unlock();
            }
        }
        break;
    }
}

#define LCD_HOST           SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)

// 170x320 resolution
#define LCD_H_RES          170
#define LCD_V_RES          320

void app_main(void)
{
    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(PIN_NUM_BK_LIGHT, 1);

    ESP_LOGI(TAG, "Initialize RC522 RST pin");
    gpio_config_t rst_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << RC522_RST)
    };
    ESP_ERROR_CHECK(gpio_config(&rst_config));
    gpio_set_level(RC522_RST, 1); // Aktif HIGH untuk RST

    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_LCD_DC,
        .cs_gpio_num = PIN_NUM_LCD_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install ST7789 panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    // ST7789 on 170x320 might require X gap of 35 pixels
    esp_lcd_panel_set_gap(panel_handle, 35, 0);

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    ESP_LOGI(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES * 80,
        .double_buffer = true,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
        }
    };
    lv_disp_t * disp = lvgl_port_add_disp(&disp_cfg);

    ESP_LOGI(TAG, "Create UI");
    
    // Acquire LVGL lock before modifying UI
    lvgl_port_lock(0);
    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    rfid_label = lv_label_create(scr);
    
    // Set text and apply a larger font if possible
    LV_FONT_DECLARE(lv_font_montserrat_48);
    lv_obj_set_style_text_font(rfid_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_align(rfid_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(rfid_label, "Scan\nRFID...");
    lv_obj_center(rfid_label);
    lvgl_port_unlock();

    ESP_LOGI(TAG, "Initialize RC522");
    rc522_config_t rc522_cfg = {
        .spi.host = RC522_SPI_HOST,
        .spi.miso_gpio = RC522_MISO,
        .spi.mosi_gpio = RC522_MOSI,
        .spi.sck_gpio = RC522_SCLK,
        .spi.sda_gpio = RC522_CS,
    };
    
    rc522_handle_t scanner;
    ESP_ERROR_CHECK(rc522_create(&rc522_cfg, &scanner));
    ESP_ERROR_CHECK(rc522_register_events(scanner, RC522_EVENT_ANY, rc522_handler, NULL));
    ESP_ERROR_CHECK(rc522_start(scanner));

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}
