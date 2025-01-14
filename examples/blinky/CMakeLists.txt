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
    LANGUAGES C ASM
)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)  # required for inline asm

# Create variables to alias path names
set(NOTE_C ${CMAKE_CURRENT_LIST_DIR}/../../note-c)
set(SRC ${CMAKE_CURRENT_LIST_DIR}/src)

# Set global compile settings
zephyr_get_compile_options_for_lang_as_string(C zephyr_options)
message(STATUS ${zephyr_options})
message(STATUS) # Insert blank line for readability
zephyr_compile_options(
    -ggdb
    -Og
    -Wall
    # -Wextra
    -Wno-unused-parameter
    # -Wpedantic
)
zephyr_get_compile_options_for_lang_as_string(C zephyr_options)
message(STATUS ${zephyr_options})
message(STATUS) # Insert blank line for readability

# Zephyr Example Application
target_sources(app
    PRIVATE ${SRC}/main.c
)

# WARNING: These options are overriden by `zephyr_compile_options()`
target_compile_options(app
    PRIVATE -ggdb
    PRIVATE -Og
    PRIVATE -Wall
    PRIVATE -Wextra
    # PRIVATE -Wpedantic
    PRIVATE -Wimplicit-fallthrough=2
    PRIVATE -Wunused-parameter
)
