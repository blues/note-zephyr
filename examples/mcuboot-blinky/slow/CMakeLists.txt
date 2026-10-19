# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
# Uncomment this to set the path of the custom signing key
#SET(mcuboot_CONFIG_BOOT_SIGNATURE_KEY_FILE "\"${CMAKE_SOURCE_DIR}/child_image/priv.pem\"")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(slow_blinky)

target_sources(app PRIVATE src/main.c)
