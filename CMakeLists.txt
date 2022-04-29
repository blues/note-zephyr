# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(note-zephyr)

set(NOTE_C ${CMAKE_CURRENT_LIST_DIR}/note-c)

target_include_directories(app PRIVATE ${NOTE_C})

# Let Zephyr build all 3rd party libs with app. Merge all into one
target_sources(app PRIVATE
    # note-c Sources
    ${NOTE_C}/n_atof.c
    ${NOTE_C}/n_b64.c
    ${NOTE_C}/n_cjson.c
    ${NOTE_C}/n_cjson_helpers.c
    ${NOTE_C}/n_const.c
    ${NOTE_C}/n_ftoa.c
    ${NOTE_C}/n_helpers.c
    ${NOTE_C}/n_hooks.c
    ${NOTE_C}/n_i2c.c
    ${NOTE_C}/n_md5.c
    ${NOTE_C}/n_printf.c
    ${NOTE_C}/n_request.c
    ${NOTE_C}/n_serial.c
    ${NOTE_C}/n_str.c
    ${NOTE_C}/n_ua.c
    # Application sources
    src/main.c)
