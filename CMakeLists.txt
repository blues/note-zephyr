# Copyright 2022 Blues Inc.  All rights reserved.
# Use of this source code is governed by licenses granted by the
# copyright holder including that found in the LICENSE file.

# SPDX-License-Identifier: MIT

# Set CMake policy behavior
cmake_minimum_required(VERSION 3.20.0)

# Load Zephyr Package
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Basic Project Configuration
project(note-zephyr
    VERSION 1.0.0
    LANGUAGES C
)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Create variables to alias path names
set(NOTE_C ${CMAKE_CURRENT_LIST_DIR}/note-c)
set(SRC ${CMAKE_CURRENT_LIST_DIR}/src)

# Set global compile settings
zephyr_get_compile_options_for_lang_as_string(C zephyr_options)
message(STATUS ${zephyr_options})
zephyr_compile_options(
    -g
    # -O0
    -Wall
    -Wextra
    # -Wpedantic
    -Wimplicit-fallthrough=2
    -Wno-unused-parameter
)
zephyr_get_compile_options_for_lang_as_string(C zephyr_options)
message(STATUS ${zephyr_options})

# # Compile `note-c` seperately

# # The note-c library that we are linking with does not know how
# # to build for this platform, so we export all the flags used in
# # this zephyr build to the note-c.
# zephyr_get_include_directories_for_lang_as_string(       C zephyr_includes)
# zephyr_get_compile_definitions_for_lang_as_string(       C zephyr_definitions)
# zephyr_get_compile_options_for_lang_as_string(           C zephyr_options)
# zephyr_get_system_include_directories_for_lang_as_string(C zephyr_system_includes)

# set(zephyr_project_cflags
#     "${zephyr_includes} ${zephyr_definitions} ${zephyr_options} ${zephyr_system_includes}"
# )
# message(STATUS ${zephyr_project_cflags})

# add_library(note-c STATIC
#     PRIVATE ${NOTE_C}/n_atof.c
#     PRIVATE ${NOTE_C}/n_b64.c
#     PRIVATE ${NOTE_C}/n_cjson.c
#     PRIVATE ${NOTE_C}/n_cjson_helpers.c
#     PRIVATE ${NOTE_C}/n_const.c
#     PRIVATE ${NOTE_C}/n_ftoa.c
#     PRIVATE ${NOTE_C}/n_helpers.c
#     PRIVATE ${NOTE_C}/n_hooks.c
#     PRIVATE ${NOTE_C}/n_i2c.c
#     PRIVATE ${NOTE_C}/n_md5.c
#     PRIVATE ${NOTE_C}/n_printf.c
#     PRIVATE ${NOTE_C}/n_request.c
#     PRIVATE ${NOTE_C}/n_serial.c
#     PRIVATE ${NOTE_C}/n_str.c
#     PRIVATE ${NOTE_C}/n_ua.c
# )

# target_compile_options(note-c
#     PRIVATE ${zephyr_project_cflags}
# )

# target_include_directories(note-c
#     PUBLIC ${NOTE_C}
# )

# Let Zephyr build additional 3rd party libs with `app`.
target_sources(app
    PRIVATE ${NOTE_C}/n_atof.c
    PRIVATE ${NOTE_C}/n_b64.c
    PRIVATE ${NOTE_C}/n_cjson.c
    PRIVATE ${NOTE_C}/n_cjson_helpers.c
    PRIVATE ${NOTE_C}/n_const.c
    PRIVATE ${NOTE_C}/n_ftoa.c
    PRIVATE ${NOTE_C}/n_helpers.c
    PRIVATE ${NOTE_C}/n_hooks.c
    PRIVATE ${NOTE_C}/n_i2c.c
    PRIVATE ${NOTE_C}/n_md5.c
    PRIVATE ${NOTE_C}/n_printf.c
    PRIVATE ${NOTE_C}/n_request.c
    PRIVATE ${NOTE_C}/n_serial.c
    PRIVATE ${NOTE_C}/n_str.c
    PRIVATE ${NOTE_C}/n_ua.c
    PRIVATE ${SRC}/main.c
    PRIVATE ${SRC}/notecard.c
)

target_include_directories(app
    PRIVATE ${NOTE_C}
)

# add_dependencies(app
#     note-c
# )

# target_link_libraries(app
#     PRIVATE note-c
# )
