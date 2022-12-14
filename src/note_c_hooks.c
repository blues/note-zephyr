#include "note_c_hooks.h"

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

static const size_t REQUEST_HEADER_SIZE = 2;
static const uint16_t SERIAL_PEEK_EMPTY_MASK = 0xFF00;

const struct device *i2c_dev = NULL;
bool i2c_initialized = false;

const struct device *serial_dev = NULL;
bool serial1_initialized = false;
uint16_t serial_peek_buffer = SERIAL_PEEK_EMPTY_MASK;

const char *note_i2c_receive(uint16_t device_address_, uint8_t *buffer_, uint16_t size_, uint32_t *available_) {
    // Let the Notecard know that we are getting ready to read some data
    uint8_t size_buf[2];
    size_buf[0] = 0;
    size_buf[1] = (uint8_t)size_;
    uint8_t write_result = i2c_write(i2c_dev, size_buf, sizeof(size_buf), device_address_);

    if (write_result != 0) {
        return "i2c: Unable to initate read from the Notecard\n";
    }

    // Read from the Notecard and copy the response bytes into the response buffer
    const int request_length = size_ + REQUEST_HEADER_SIZE;
    uint8_t read_buf[256];
    uint8_t read_result = i2c_read(i2c_dev, read_buf, request_length, device_address_);

    if (read_result != 0) {
        return "i2c: Unable to receive data from the Notecard.\n";
    } else {
        *available_ = (uint32_t)read_buf[0];
        uint8_t bytes_to_read = read_buf[1];
        for (size_t i = 0; i < bytes_to_read; i++) {
            buffer_[i] = read_buf[i + 2];
        }

        return NULL;
    }
}

bool note_i2c_reset(uint16_t device_address_) {
    (void)device_address_;

    if (i2c_initialized) {
        return true;
    }

    if (!i2c_dev) {
        i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
    }

    if (!device_is_ready(i2c_dev)) {
        printk("i2c: Device is not ready.\n");
        return false;
    }

    printk("i2c: Device is ready.\n");

    i2c_initialized = true;
    return true;
}

const char *note_i2c_transmit(uint16_t device_address_, uint8_t *buffer_, uint16_t size_) {
    // Create a buffer that contains the number of bytes and the data to write to the Notecard
    uint8_t write_buf[size_ + 1];
    write_buf[0] = (uint8_t)size_;
    for (size_t i = 0; i < size_; i++) {
        write_buf[i + 1] = buffer_[i];
    }

    // Write the message
    uint8_t write_result = i2c_write(i2c_dev, write_buf, sizeof(write_buf), device_address_);

    if (write_result != 0) {
        return "i2c: Unable to transmit data to the Notecard\n";
    } else {
        return NULL;
    }
}

size_t note_log_print(const char *message_) {
    if (message_) {
        printk("%s", message_);
        return 1;
    }

    return 0;
}

bool note_serial_available(void)
{
    bool result;

    if (SERIAL_PEEK_EMPTY_MASK & serial_peek_buffer)
    {
        unsigned char next_char;
        int status = uart_poll_in(serial_dev, &next_char);
        switch (status)
        {
        case 0:
            serial_peek_buffer = next_char;
            result = true;
            break;
        case -1:
            // No character was available to read
            // fallthrough
        case -ENOSYS:
            // The operation was not implemented
            // fallthrough
        case -EBUSY:
            // Async reception was enabled using `uart_rx_enable`
            // fallthrough
        default:
            result = false;
            break;
        }
    }
    else
    {
        result = true;
    }

    return result;
}

char note_serial_receive(void)
{
    char result;

    if (!(SERIAL_PEEK_EMPTY_MASK & serial_peek_buffer))
    {
        // Peek buffer is NOT empty
        result = serial_peek_buffer;
        serial_peek_buffer = SERIAL_PEEK_EMPTY_MASK;
    }
    else if (uart_poll_in(serial_dev, (unsigned char *)&result))
    {
        result = '\0';
    }

    return result;
}

bool note_serial_reset(void)
{
    bool result;

    if (serial1_initialized)
    {
        // Empty buffer
        for (char c; !uart_poll_in(serial_dev, &c);)
            ;
        result = true;
    }
    else
    {
        if (!serial_dev)
        {
            serial_dev = DEVICE_DT_GET(DT_NODELABEL(usart1));
        }
        struct uart_config serial_cfg = {
            .baudrate = 9600,
            .parity = UART_CFG_PARITY_NONE,
            .stop_bits = UART_CFG_STOP_BITS_1,
            .data_bits = UART_CFG_DATA_BITS_8,
            .flow_ctrl = UART_CFG_FLOW_CTRL_NONE};
        if (uart_configure(serial_dev, &serial_cfg))
        {
            printk("serial: Failed to configure device.\n");
            serial1_initialized = false;
            result = false;
        }
        else if (!device_is_ready(serial_dev))
        {
            printk("serial: Device is not ready.\n");
            serial1_initialized = false;
            result = false;
        }
        else
        {
            printk("serial: Device is ready.\n");
            serial1_initialized = true;
            result = true;
        }
    }

    return result;
}

void note_serial_transmit(uint8_t *text_, size_t len_, bool flush_)
{
    (void)flush_; // `uart_poll_out` blocks (i.e. always flushes)

    for (size_t i = 0; i < len_; ++i)
    {
        uart_poll_out(serial_dev, text_[i]);
    }
}

void platform_delay(uint32_t ms) {
    k_msleep(ms);
}

uint32_t platform_millis(void) {
    return (uint32_t)k_uptime_get();
}
