#ifndef NOTE_C_HOOKS_H
#define NOTE_C_HOOKS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

const char *note_i2c_receive(uint16_t device_address_, uint8_t *buffer_, uint16_t size_, uint32_t *available_);
bool note_i2c_reset(uint16_t device_address_);
const char *note_i2c_transmit(uint16_t device_address_, uint8_t *buffer_, uint16_t size_);

size_t note_log_print(const char *message_);

bool note_serial_available(void);
char note_serial_receive(void);
bool note_serial_reset(void);
void note_serial_transmit(uint8_t *text_, size_t len_, bool flush_);

void platform_delay(uint32_t ms);
uint32_t platform_millis(void);

#endif // NOTE_C_HOOKS_H
