# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(note-zephyr)

target_sources(app PRIVATE src/main.c)

# The external static library that we are linking with does not know
# how to build for this platform so we export all the flags used in
# this zephyr build to the external build system.
#
# Other external build systems may be self-contained enough that they
# do not need any build information from zephyr. Or they may be
# incompatible with certain zephyr options and need them to be
# filtered out.
zephyr_get_include_directories_for_lang_as_string(       C includes)
zephyr_get_system_include_directories_for_lang_as_string(C system_includes)
zephyr_get_compile_definitions_for_lang_as_string(       C definitions)
zephyr_get_compile_options_for_lang_as_string(           C options)

set(external_project_cflags
  "${includes} ${definitions} ${options} ${system_includes}"
)

include(ExternalProject)

set(NOTE_C ${CMAKE_CURRENT_LIST_DIR}/note-c)

# Compile `note-c` seperately
add_library(note-c STATIC
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
)

target_include_directories(note-c
    PUBLIC ${NOTE_C}
)

# Create a wrapper CMake library that our app can link with
add_dependencies(app
  note-c
)

target_link_libraries(app PUBLIC note-c)
