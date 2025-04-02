zephyr_library()

zephyr_library_sources(
	../note-c/n_atof.c
	../note-c/n_b64.c
	../note-c/n_cjson.c
	../note-c/n_cjson_helpers.c
	../note-c/n_cobs.c
	../note-c/n_const.c
	../note-c/n_ftoa.c
	../note-c/n_helpers.c
	../note-c/n_hooks.c
	../note-c/n_i2c.c
	../note-c/n_md5.c
	../note-c/n_printf.c
	../note-c/n_request.c
	../note-c/n_serial.c
	../note-c/n_str.c
	../note-c/n_ua.c
)

zephyr_include_directories(
	.
	../note-c/
)

add_subdirectory(hooks)
add_subdirectory(transport)

if(CONFIG_BLUES_NOTEHUB_PRODUCT_UID STREQUAL "")
message(WARNING "Product UID is not set. Please ensure your Notecard has a product identifier with a value set before running this example. Set it with CONFIG_BLUES_NOTEHUB_PRODUCT_UID in your prj.conf file.")
endif()
