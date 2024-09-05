/*
This example is based on WT32-SC01-Plus in ESP-IDF environment.
Using BSP-IDF5-ESP_LCD-LVGL9 [https://github.com/sukesh-ak/BSP-IDF5-ESP_LCD-LVGL9] 

$ Tested version
ESP-IDF: 5.3.0
LVGL: 9.2.0
esp_lvgl_port: 2.3.1
esp_lcd_st7796: 1.3.0
esp_lcd_touch_ft5x06: 1.0.6
*/

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "esp_lcd_types.h"
#include "esp_lcd_st7796.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_touch_ft5x06.h"
#include "lvgl.h"
#include "lvgl_joystick.h"

static const char *TAG = "WT32-SC01_Plus";

#define BSP_I2C_SCL           (GPIO_NUM_5)
#define BSP_I2C_SDA           (GPIO_NUM_6)

/* LCD display color format */
#define BSP_LCD_COLOR_FORMAT        (1) //RGB565

/* LCD display color bytes endianess */
#define BSP_LCD_BIGENDIAN           (1)

/* LCD display color bits */
#define BSP_LCD_BITS_PER_PIXEL      (16)

/* LCD display color space */
#define BSP_LCD_COLOR_SPACE         (ESP_LCD_COLOR_SPACE_BGR)

/* LCD definition */
#define BSP_LCD_H_RES              (320)
#define BSP_LCD_V_RES              (480)

#define BSP_LCD_PIXEL_CLOCK_HZ     (40 * 1000 * 1000)

/* Display - ST7789 8 Bit parellel */
#define BSP_LCD_WIDTH         (8) 
#define BSP_LCD_DATA0         (GPIO_NUM_9)
#define BSP_LCD_DATA1         (GPIO_NUM_46)
#define BSP_LCD_DATA2         (GPIO_NUM_3)
#define BSP_LCD_DATA3         (GPIO_NUM_8)
#define BSP_LCD_DATA4         (GPIO_NUM_18)
#define BSP_LCD_DATA5         (GPIO_NUM_17)
#define BSP_LCD_DATA6         (GPIO_NUM_16)
#define BSP_LCD_DATA7         (GPIO_NUM_15)

#define BSP_LCD_CS           (GPIO_NUM_NC) 
#define BSP_LCD_DC           (GPIO_NUM_0)  
#define BSP_LCD_WR           (GPIO_NUM_47) 
#define BSP_LCD_RD           (GPIO_NUM_NC) 
#define BSP_LCD_RST          (GPIO_NUM_4)  
#define BSP_LCD_TE           (GPIO_NUM_48) 
#define BSP_LCD_BACKLIGHT    (GPIO_NUM_45) 
#define BSP_LCD_TP_INT       (GPIO_NUM_7)  
#define BSP_LCD_TP_RST       (GPIO_NUM_NC) 

#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8
#define LCD_LEDC_CH            1
#define BSP_I2C_NUM            1
#define BSP_I2C_CLK_SPEED_HZ   400000


#define Movement_Factor 4 // You can adjust the amount of movement.

static esp_lcd_touch_handle_t tp;   // LCD touch handle
static esp_lcd_panel_handle_t panel_handle = NULL;

/* Black Dot */
lv_obj_t *target;

esp_err_t bsp_i2c_init(void) {
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BSP_I2C_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = BSP_I2C_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = BSP_I2C_CLK_SPEED_HZ
    };
    /* Initialize I2C */
    ESP_ERROR_CHECK(i2c_param_config(BSP_I2C_NUM, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(BSP_I2C_NUM, i2c_conf.mode, 0, 0, 0));
    ESP_LOGI(TAG, "Initialize touch IO (I2C)");

    return ESP_OK;
}

esp_err_t bsp_i2c_deinit(void) {
    ESP_ERROR_CHECK(i2c_driver_delete(BSP_I2C_NUM));
    ESP_LOGI(TAG, "De-Initialize touch IO (I2C)");
    return ESP_OK;
}

esp_err_t bsp_touch_new()
{
    /* Initilize I2C */
    ESP_ERROR_CHECK(bsp_i2c_init()); 
    /* Initialize touch */
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = BSP_LCD_V_RES,
        .y_max = BSP_LCD_H_RES,
        .rst_gpio_num = BSP_LCD_TP_RST, // Shared with LCD reset
        .int_gpio_num = BSP_LCD_TP_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    const esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)BSP_I2C_NUM, &tp_io_config, &tp_io_handle));
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &tp));
    assert(tp);

    return ESP_OK;
}

lv_indev_t *bsp_display_indev_init(lv_display_t *disp) {

    ESP_ERROR_CHECK(bsp_touch_new(NULL, &tp));
    assert(tp);

    /* Add touch input (for selected screen) */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = tp,
    };

    return lvgl_port_add_touch(&touch_cfg);    
}

static esp_err_t bsp_display_brightness_init(void)
{
    // Setup LEDC peripheral for PWM backlight control
    const ledc_channel_config_t LCD_backlight_channel = {
        .gpio_num = BSP_LCD_BACKLIGHT,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LCD_LEDC_CH,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = 1,
        .duty = 0,
        .hpoint = 0
    };
    const ledc_timer_config_t LCD_backlight_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = 1,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&LCD_backlight_timer));
    ESP_ERROR_CHECK(ledc_channel_config(&LCD_backlight_channel));

    return ESP_OK;
}

esp_err_t bsp_display_brightness_set(int percent)
{
    if (percent > 100) {
        percent = 100;
    }
    if (percent < 0) {
        percent = 0;
    }

    ESP_LOGI(TAG, "Setting LCD backlight: %d%%", percent);
    uint32_t duty_cycle = (1023 * percent) / 100; 
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LCD_LEDC_CH, duty_cycle));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LCD_LEDC_CH));

    return ESP_OK;
}

void display_start(void) 
{
    /* Init Intel 8080 bus */
    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    esp_lcd_i80_bus_config_t bus_config = {
        .clk_src = LCD_CLK_SRC_PLL160M,
        .dc_gpio_num = BSP_LCD_DC,
        .wr_gpio_num = BSP_LCD_WR,
        .data_gpio_nums = {
            BSP_LCD_DATA0,
            BSP_LCD_DATA1,
            BSP_LCD_DATA2,
            BSP_LCD_DATA3,
            BSP_LCD_DATA4,
            BSP_LCD_DATA5,
            BSP_LCD_DATA6,
            BSP_LCD_DATA7,
        },
        .bus_width = BSP_LCD_WIDTH,
        .max_transfer_bytes = (BSP_LCD_H_RES) * 128 * sizeof(uint16_t),
        .psram_trans_align = 64,
        .sram_trans_align = 4,
    };
    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

    ESP_LOGD(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = BSP_LCD_CS,
        .pclk_hz = BSP_LCD_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 10,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
        .flags = {
            .swap_color_bytes = 1,
            .pclk_idle_low = 0,
        },
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle));

    ESP_LOGD(TAG, "Install LCD driver of ST7796");
        esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BSP_LCD_RST,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    
    // Set inversion, x/y coordinate order, x/y mirror according to your LCD module spec
    // the gap is LCD panel specific, even panels with the same driver IC, can have different gap value
    esp_lcd_panel_invert_color(panel_handle, true);
    esp_lcd_panel_mirror(panel_handle, true, false);

    // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
     const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = (BSP_LCD_H_RES * BSP_LCD_V_RES) /10,
        .trans_size = BSP_LCD_H_RES * BSP_LCD_V_RES,
        .double_buffer = true,
        .hres = BSP_LCD_H_RES,
        .vres = BSP_LCD_V_RES,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,            
        }
    };

    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));
    lv_display_t * disp = lvgl_port_add_disp(&disp_cfg);
    bsp_display_indev_init(disp);

    lv_disp_set_rotation(disp, LV_DISP_ROTATION_180);
    ESP_ERROR_CHECK(bsp_display_brightness_init());
}

esp_err_t bsp_display_on(void)
{
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    return ESP_OK;
}

// Example callback function
void joystick_position_callback(uint8_t joystick_id, int16_t x, int16_t y) {
  printf("Joystick ID: %d, Position - X: %d, Y: %d\n", joystick_id, x, y);

  int16_t _x = lv_obj_get_x_aligned(target) + (x * Movement_Factor);
  int16_t _y = lv_obj_get_y_aligned(target) + (y * Movement_Factor);
  lv_obj_set_pos(target, _x, _y);
}

/* Entry point to LVGL UI */
void ui_init()
{
// Main screen style
  static lv_style_t style_base;
  lv_style_init(&style_base);
  lv_style_set_border_width(&style_base, 0);

  // Main screen
  lv_obj_t *screen = lv_obj_create(lv_screen_active());
  lv_obj_set_size(screen, BSP_LCD_H_RES, BSP_LCD_V_RES);
  lv_obj_center(screen);
  lv_obj_add_style(screen, &style_base, LV_PART_MAIN);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  // Moving target style
  static lv_style_t style_target;
  lv_style_init(&style_target);
  lv_style_set_border_width(&style_target, 4);
  lv_style_set_radius(&style_target, 20);
  lv_style_set_bg_color(&style_target, lv_color_black());

  // Moving target object
  target = lv_obj_create(screen);
  lv_obj_set_size(target, 40, 40);
  lv_obj_set_pos(target, BSP_LCD_H_RES / 2 - 30, BSP_LCD_V_RES / 2 - 40);
  lv_obj_add_style(target, &style_target, LV_PART_MAIN);

  // Create joystick
  create_joystick(screen, 10, LV_ALIGN_BOTTOM_MID, 0, 0, 100, 25, NULL, NULL, joystick_position_callback);

  printf("UI DRAW Done\n");
}

void app_main(void)
{
    display_start();
    bsp_display_brightness_set(100);
    ui_init();
    bsp_display_on();
}
