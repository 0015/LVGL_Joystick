cmake_minimum_required(VERSION 3.5)

set(app_sources "src/lvgl_joystick.c")

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "src/"
    REQUIRES lvgl)

component_compile_options(-Wno-error=format= -Wno-format)


project(LVGL_Joystick)