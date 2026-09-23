# Copyright (c) 2025 Blues Inc.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

# Generate blues_logo.h from the PNG at configure time.
# PYTHON_EXECUTABLE is set by Zephyr's CMake modules; fall back to finding
# Python3 explicitly if this file is ever used outside a Zephyr build.
if(NOT PYTHON_EXECUTABLE)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
endif()

set(BLUES_LOGO_H ${CMAKE_CURRENT_BINARY_DIR}/blues_logo.h)
execute_process(
    COMMAND ${PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/png_to_header.py
            ${CMAKE_CURRENT_SOURCE_DIR}/blues_logo.png
            ${BLUES_LOGO_H}
            blues_logo_png
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to generate blues_logo.h from blues_logo.png")
endif()
target_include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_sources(app PRIVATE src/main.c)
