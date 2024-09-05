/*
This example is based on WT32-SC01-Plus in Arduino environment. The graphics library uses LovyanGFX.

$ Tested version
Arduino ESP32: 3.0.4
LVGL: 9.2.0
LovyanGFX: 1.1.16
*/

#include "LGFX_WT32-SC01-Plus.h"
#include <lvgl.h>
#include <lvgl_joystick.h>

static LGFX tft;

#define TFT_HOR_RES 320
#define TFT_VER_RES 480
#define Movement_Factor 4 // You can adjust the amount of movement.

/* LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes */
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
void *draw_buf_1;

/* Black Dot */
lv_obj_t *target;

/* Display flushing */
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.writePixels((lgfx::rgb565_t *)px_map, w * h);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  uint16_t touchX, touchY;
  bool touched = tft.getTouch(&touchX, &touchY);
  if (!touched) {
    data->state = LV_INDEV_STATE_RELEASED;
  } else {
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = touchX;
    data->point.y = touchY;
  }
}

uint32_t millis_cb(void) {
  return millis();
}

void setup() {
  Serial.begin(115200);

  display_init();

  lv_init();

  lv_tick_set_cb(millis_cb);

  lv_display_t *disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
  draw_buf_1 = heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_display_set_buffers(disp, draw_buf_1, NULL, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);

  ui_init();
}

void loop() {
  lv_task_handler();
}

void display_init() {
  tft.begin();
  tft.setRotation(2);
  tft.setBrightness(255);
}

// Example callback function
void joystick_position_callback(uint8_t joystick_id, int16_t x, int16_t y) {
  Serial.printf("Joystick ID: %d, Position - X: %d, Y: %d\n", joystick_id, x, y);

  int16_t _x = lv_obj_get_x_aligned(target) + (x * Movement_Factor);
  int16_t _y = lv_obj_get_y_aligned(target) + (y * Movement_Factor);
  lv_obj_set_pos(target, _x, _y);
}

void ui_init() {
  // Main screen style
  static lv_style_t style_base;
  lv_style_init(&style_base);
  lv_style_set_border_width(&style_base, 0);

  // Main screen
  lv_obj_t *screen = lv_obj_create(lv_screen_active());
  lv_obj_set_size(screen, TFT_HOR_RES, TFT_VER_RES);
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
  lv_obj_set_pos(target, TFT_HOR_RES / 2 - 30, TFT_VER_RES / 2 - 40);
  lv_obj_add_style(target, &style_target, LV_PART_MAIN);

  // Create joystick
  create_joystick(screen, 10, LV_ALIGN_BOTTOM_MID, 0, 0, 100, 25, NULL, NULL, joystick_position_callback);

  Serial.println("UI DRAW Done");
}