#include <stdlib.h>
#include <zephyr.h>

extern const struct device *i2c_dev;

static uint32_t platform_millis(void)
{
	return (uint32_t) k_uptime_get();
}

static void platform_delay(uint32_t ms)
{
	k_msleep(ms);
}

const char *noteI2cReceive(uint16_t device_address_, uint8_t *buffer_, uint16_t size_, uint32_t *available_)
{
    uint8_t read_result = i2c_read(i2c_dev, buffer_, size_, device_address_);

    if (read_result) {
        return "i2c: Unable to receive data from the Notecard.\n";
    } else {
        return buffer_;
    }
}

/*
 * Reset the Notecard by writing a zero and reading the response.
 * If the Notecard provides a zero-length reply, it's ready for us to use.
 */
bool noteI2cReset(uint16_t device_address_)
{
    if (!i2c_dev) {
        i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
    }

	if (!device_is_ready(i2c_dev)) {
		printk("i2c: Device is not ready.\n");
		return false;
	} else {
        printk("i2c: Device is ready.\n");
        return true;
    }

    uint16_t write_ret;
    uint16_t read_ret;

    uint8_t chunk_len = 0;
    uint8_t write_buf[2];
    uint8_t read_buf[2];

    write_buf[0] = 0x00;
    write_buf[1] = chunk_len;

	write_ret = i2c_write(i2c_dev, write_buf, 2, device_address_);

	if (write_ret == 0) {
        // Read two bytes from the Notecard
        read_ret = i2c_read(i2c_dev, read_buf, 2, device_address_);

        // If the Notecard returns 0 bytes available, it's reset and ready.
        // Otherwise, we need to try again.
        if (read_buf[0] == 0) {
            printk("i2c: Notecard at 0x%x successfully reset\n", device_address_);
            return true;
        } else {
            printk("i2c: Notecard not reset. Trying again...\n");
            noteI2cReset(device_address_);
        }
    }

    // Reset failed, return false
    printk("i2c: Unable to communicate with the Notecard");
    return false;
}

const char *noteI2cTransmit(uint16_t device_address_, uint8_t *buffer_, uint16_t size_)
{
    uint8_t write_result = i2c_write(i2c_dev, buffer_, size_, device_address_);

    if (write_result) {
        return "i2c: Unable to transmit data to the Notecard\n";
    } else {
        return buffer_;
    }
}

size_t noteLogPrint(const char * message_)
{
    if (message_) {
        printk("%s", message_);
        return 1;
    }

    return 0;
}