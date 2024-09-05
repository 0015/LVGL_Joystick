# LVGL Joystick Library

This library provides an easy way to create virtual joysticks for use in an LVGL environment. It supports both ESP-IDF and Arduino platforms and allows you to handle joystick input with customizable styles and callbacks.

[![Virtual Joystic for LVGL](./misc/demo_1.gif)](https://youtu.be/QYOfr-VTves)

## Features

- Create virtual joysticks with customizable sizes and styles.
- Handle joystick movement with user-defined callbacks.
- Allows creation of multiple virtual joysticks, identifiable via ID.
- Supports both ESP-IDF and Arduino environments.

## Installation

### Arduino

**Install the Joystick Library**:
   - Download or clone this repository.
   - Copy the `LVGL_Joystick` folder to your Arduino `libraries` directory.

### ESP-IDF

**Add Joystick Library as a component**:
   - Clone the LVGL Joystick repository into your `components/` directory:

```bash
mkdir -p components
cd components
git clone https://github.com/0015/LVGL_Joystick
```

## Usage
```cpp   
    #include <lvgl.h>
    #include <joystick.h>

    void joystick_position_callback(uint8_t joystick_id, int16_t x, int16_t y) {
        Serial.printf("Joystick ID: %d, Position - X: %d, Y: %d\n", joystick_id, x, y);
    }

    void ui_init() {
        lv_obj_t *screen = lv_scr_act();
        create_joystick(screen, 1, LV_ALIGN_CENTER, 0, 0, 100, 25, NULL, NULL, joystick_position_callback);
    }

```
**You can check more details in the example project.**


## Limitations

Since LVGL (the current version 9.2.0) does not support multi-touch, you cannot fire two touch events at the same time. More than two joysticks will be available as soon as LVGL is updated.