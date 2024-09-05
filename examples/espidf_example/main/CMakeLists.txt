idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES "spi_flash esp_lcd esp_lcd_st7796 esp_lvgl_port lvgl esp_timer esp_lcd_touch LVGL_Joystick"
                    PRIV_REQUIRES "driver")
                    