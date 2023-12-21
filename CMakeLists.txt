if(CONFIG_NOTECARD)
  set(NOTE_C_DIR ${ZEPHYR_CURRENT_MODULE_DIR}/note-c)
  zephyr_include_directories(. ${NOTE_C_DIR})
  zephyr_library_sources(
    note_c_hooks.c
    ${NOTE_C_DIR}/n_atof.c
    ${NOTE_C_DIR}/n_atof.c
    ${NOTE_C_DIR}/n_b64.c
    ${NOTE_C_DIR}/n_cjson.c
    ${NOTE_C_DIR}/n_cjson_helpers.c
    ${NOTE_C_DIR}/n_cobs.c
    ${NOTE_C_DIR}/n_const.c
    ${NOTE_C_DIR}/n_ftoa.c
    ${NOTE_C_DIR}/n_helpers.c
    ${NOTE_C_DIR}/n_hooks.c
    ${NOTE_C_DIR}/n_i2c.c
    ${NOTE_C_DIR}/n_md5.c
    ${NOTE_C_DIR}/n_printf.c
    ${NOTE_C_DIR}/n_request.c
    ${NOTE_C_DIR}/n_serial.c
    ${NOTE_C_DIR}/n_str.c
    ${NOTE_C_DIR}/n_ua.c
  )
endif()
