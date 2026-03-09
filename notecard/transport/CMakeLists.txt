zephyr_library_sources_ifdef(CONFIG_BLUES_NOTECARD_TRANSPORT_I2C
	transport_i2c.c
)

zephyr_library_sources_ifdef(CONFIG_BLUES_NOTECARD_TRANSPORT_UART
	transport_uart.c
)

if(CONFIG_BLUES_NOTECARD_TRANSPORT_USB_SERIAL)
	zephyr_library_sources(transport_usb_serial.c)
	# Include internal Zephyr USBH headers required by the CDC-ACM class driver.
	# These provide usbh_device.h, usbh_ch9.h, and usbh_desc.h.
	zephyr_include_directories(${ZEPHYR_BASE}/subsys/usb/host)
endif()

