# Copyright (c) 2024 Croxel Inc.
# SPDX-License-Identifier: MIT

zephyr_library_sources_ifdef(CONFIG_BLUES_NOTECARD_TRANSPORT_I2C
	transport_i2c.c
)

zephyr_library_sources_ifdef(CONFIG_BLUES_NOTECARD_TRANSPORT_UART
	transport_uart.c
)
