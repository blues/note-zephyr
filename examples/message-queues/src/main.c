#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <note.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define BUTTON_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(BUTTON_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
#define BUTTON_PIN  DT_GPIO_PIN(BUTTON_NODE, gpios)

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(BUTTON_NODE, gpios,
							      {0});
static struct gpio_callback button_cb_data;

#define STACK_SIZE 1024
#define PRIORITY 5
#define MSG_Q_SIZE 10

K_MSGQ_DEFINE(button_msgq, sizeof(uint32_t), MSG_Q_SIZE, 4);
K_THREAD_STACK_DEFINE(process_stack, STACK_SIZE);

static struct k_thread process_thread;
static volatile uint32_t button_counter = 0;

static void process_button_thread(void *dummy1, void *dummy2, void *dummy3)
{
    uint32_t count;
    J *req = NULL;
    J *body = NULL;
    int ret;

    ARG_UNUSED(dummy1);
    ARG_UNUSED(dummy2);
    ARG_UNUSED(dummy3);

    while (1) {
        // Wait for message with timeout
        ret = k_msgq_get(&button_msgq, &count, K_MSEC(1000));

        if (ret == -EAGAIN) {
            continue;  // Timeout occurred
        }

        if (ret != 0) {
            LOG_ERR("Error receiving message: %d", ret);
            continue;
        }

        // Process the button press
        button_counter += count;
        LOG_INF("Counter value: %d", button_counter);

        // Create the request
        req = NoteNewRequest("note.add");
        if (!req) {
            LOG_ERR("Failed to allocate request");
            continue;
        }

        // Create the body
        body = JCreateObject();
        if (!body) {
            LOG_ERR("Failed to allocate body");
            continue;
        }

        // Build the request
        JAddStringToObject(req, "file", "button.qo");
        JAddNumberToObject(body, "counter", button_counter);
        JAddItemToObject(req, "body", body);

        // Send the request
        if (!NoteRequest(req)) {
            LOG_ERR("Failed to add note.");
        }
    }
}

// GPIO interrupt callback
static void button_pressed(const struct device *dev, struct gpio_callback *cb,
                          uint32_t pins)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    // Send message to queue instead of scheduling work
    uint32_t count = 1;
    if (k_msgq_put(&button_msgq, &count, K_NO_WAIT) != 0) {
        LOG_ERR("Failed to queue button press");
    }
}

int init_notecard(void)
{
    J *req = NULL;

    NoteSetUserAgent((char *)"note-zephyr");

    req = NoteNewRequest("hub.set");
    if (!req) {
        LOG_ERR("Failed to allocate memory for hub.set request");
        return -ENOMEM;
    }

    JAddStringToObject(req, "product", CONFIG_BLUES_NOTEHUB_PRODUCT_UID);
    JAddStringToObject(req, "mode", "continuous");
    JAddStringToObject(req, "sn", "zephyr-work-queue");

    if (!NoteRequest(req)) {
        LOG_ERR("Failed to configure Notecard.");
        return -EIO;
    }

    return 0;
}

int main(void)
{
    int ret;

    // Initialize Notecard
    ret = init_notecard();
    if (ret != 0) {
        LOG_ERR("Failed to initialize Notecard: %d", ret);
        return ret;
    }

    // Initialize button GPIO
    ret = !gpio_is_ready_dt(&button);
    if (ret != 0) {
        LOG_ERR("Error: button device %s is not ready\n",
		       button.port->name);
        return ret;
    }

    // Configure button GPIO
    ret = gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);
    if (ret != 0) {
        LOG_ERR("Error: failed to configure button pin: %d", ret);
        return ret;
    }

    // Start processing thread
    k_thread_create(&process_thread, process_stack, STACK_SIZE,
                   process_button_thread, NULL, NULL, NULL,
                   PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&process_thread, "button_process");

    // Configure button interrupt
    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        LOG_ERR("Error: failed to configure button interrupt: %d", ret);
        return ret;
    }

    // Set up GPIO callback
    gpio_init_callback(&button_cb_data, button_pressed, BIT(BUTTON_PIN));
    ret = gpio_add_callback(button.port, &button_cb_data);
    if (ret != 0) {
        LOG_ERR("Error: failed to add button callback: %d", ret);
        return ret;
    }

    LOG_INF("Button handler initialized. Press the button to increment counter.");

    // Wait for button presses
    while (1) {
        k_sleep(K_FOREVER);
    }

    return 0;
}
