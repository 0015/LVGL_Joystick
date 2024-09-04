#include "lvgl_joystick.h"
#include <stdlib.h>

// Event handler for the joystick
static void joystic_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);

  // Retrieve joystick data
  joystick_data_t *joystick_data = (joystick_data_t *)lv_obj_get_user_data(obj);

  if (joystick_data == NULL) {
    return;  // Handle error case
  }

  uint8_t joystick_id = joystick_data->joystick_id;
  uint8_t base_radius = joystick_data->base_radius;
  uint8_t stick_radius = joystick_data->stick_radius;

  if (code == LV_EVENT_RELEASED) {
    lv_obj_set_pos(obj, 0, 0);
  }

  if (code == LV_EVENT_PRESSING) {
    lv_indev_t *indev = lv_indev_active();
    if (indev == NULL) return;

    lv_point_t vect;
    lv_indev_get_vect(indev, &vect);

    int32_t x = lv_obj_get_x_aligned(obj) + vect.x;
    int32_t y = lv_obj_get_y_aligned(obj) + vect.y;

    float distance_from_center = sqrt(x * x + y * y);
    if (distance_from_center < base_radius - (stick_radius * 1.2)) {
      lv_obj_set_pos(obj, x, y);

      if (joystick_data->position_callback) {
        joystick_data->position_callback(joystick_data->joystick_id, vect.x, vect.y);
        }
    }


  }
}

// Function to create a joystick with custom parameters
void create_joystick(lv_obj_t *parent, uint8_t joystick_id, lv_align_t base_align, int base_x, int base_y, int base_radius, int stick_radius, lv_style_t *base_style, lv_style_t *stick_style, joystick_position_cb_t position_callback) {
    // Allocate and initialize joystick data
    joystick_data_t *joystick_data = (joystick_data_t *)malloc(sizeof(joystick_data_t));
    joystick_data->joystick_id = joystick_id;
    joystick_data->base_radius = base_radius;
    joystick_data->stick_radius = stick_radius;
    joystick_data->position_callback = position_callback;

    // Create or use provided base style
    static lv_style_t default_base_style;
    if (base_style == NULL) {
        base_style = &default_base_style;
        lv_style_init(base_style);
        lv_style_set_radius(base_style, base_radius);
        lv_style_set_bg_opa(base_style, LV_OPA_COVER);
        lv_style_set_bg_color(base_style, lv_palette_lighten(LV_PALETTE_GREY, 1));
        lv_style_set_pad_all(base_style, 0);
        lv_style_set_outline_width(base_style, 2);
        lv_style_set_outline_color(base_style, lv_palette_main(LV_PALETTE_BLUE));
        lv_style_set_outline_pad(base_style, 8);
    }

    // Create the base object (joystick background)
    lv_obj_t *base_obj = lv_obj_create(parent);
    lv_obj_add_style(base_obj, base_style, LV_PART_MAIN);
    lv_obj_clear_flag(base_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(base_obj, base_radius * 2, base_radius * 2);  // Set size based on radius
    lv_obj_align(base_obj, base_align, base_x, base_y);  // Align the base_obj

    // Create or use provided stick style
    static lv_style_t default_stick_style;
    if (stick_style == NULL) {
        stick_style = &default_stick_style;
        lv_style_init(stick_style);
        lv_style_set_radius(stick_style, stick_radius);
        lv_style_set_bg_opa(stick_style, LV_OPA_COVER);
        lv_style_set_bg_color(stick_style, lv_palette_main(LV_PALETTE_BLUE));
        lv_style_set_pad_all(stick_style, 0);
        lv_style_set_outline_width(stick_style, 2);
        lv_style_set_outline_color(stick_style, lv_palette_main(LV_PALETTE_GREEN));
        lv_style_set_outline_pad(stick_style, 4);
    }

    // Create the stick object (control handle)
    lv_obj_t *stick_obj = lv_btn_create(base_obj);
    lv_obj_set_size(stick_obj, stick_radius * 2, stick_radius * 2);  // Set size based on radius
    lv_obj_add_style(stick_obj, stick_style, LV_PART_MAIN);
    lv_obj_center(stick_obj);  // Center the stick inside the base object

    // Set the joystick data as user data for the stick object
    lv_obj_set_user_data(stick_obj, joystick_data);

    // Set the event handler for the stick object
    lv_obj_add_event_cb(stick_obj, joystic_event_handler, LV_EVENT_ALL, NULL);
}
