/**
 * Copyright (c) 2025 Blues Inc.
 *
 * MIT License. Use of this source code is governed by licenses granted
 * by the copyright holder including that found in the LICENSE file.
 *
 * USB Serial (CDC-ACM host) transport for the Notecard.
 *
 * This transport communicates with the Notecard via its USB port, using
 * Zephyr's experimental USB host stack (USBH) with a CDC-ACM class driver.
 * The Notecard presents itself as a USB CDC-ACM device, and this transport
 * acts as the USB host, enabling higher-throughput communication compared
 * to the 9600-baud UART interface -- particularly useful for binary data
 * transfers via NoteBinaryStoreTransmit().
 *
 * NOTE: This transport uses Zephyr's experimental USB host stack
 * (CONFIG_USB_HOST_STACK). The application must define a USB host controller
 * using USBH_CONTROLLER_DEFINE() and ensure the underlying hardware supports
 * USB OTG host mode.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/logging/log.h>
#include <note.h>

/* Internal Zephyr USBH headers -- these are in subsys/usb/host/ */
#include "usbh_device.h"
#include "usbh_ch9.h"
#include "usbh_desc.h"

LOG_MODULE_REGISTER(notecard_usb_serial, CONFIG_BLUES_NOTECARD_USB_SERIAL_LOG_LEVEL);

/* USB CDC class codes */
#define USB_CDC_CLASS          0x02u
#define USB_CDC_ACM_SUBCLASS   0x02u
#define USB_CDC_DATA_CLASS     0x0Au

/* CDC ACM control requests (USB Class-Specific Request Codes) */
#define CDC_REQ_SET_LINE_CODING       0x20u
#define CDC_REQ_SET_CTRL_LINE_STATE   0x22u

/* bmRequestType for CDC class interface requests (host-to-device) */
#define CDC_REQTYPE_HOST_TO_IF \
	((USB_REQTYPE_DIR_TO_DEVICE << 7) | \
	 (USB_REQTYPE_TYPE_CLASS    << 5) | \
	  USB_REQTYPE_RECIPIENT_INTERFACE)

/* Bulk transfer size -- USB FS bulk max packet is 64 bytes; we request
 * larger transfers and let the UHC driver handle the segmentation.
 */
#define USB_SERIAL_BULK_BUF_SIZE  512u

/* CDC Line Coding structure (7 bytes, per USB CDC spec Table 17) */
struct cdc_line_coding {
	uint32_t dwDTERate;   /* Baud rate in bits per second */
	uint8_t  bCharFormat; /* Stop bits: 0=1, 1=1.5, 2=2 */
	uint8_t  bParityType; /* Parity: 0=None, 1=Odd, 2=Even */
	uint8_t  bDataBits;   /* Data bits: 5, 6, 7, 8, or 16 */
} __packed;

/* Runtime state for the USB serial transport */
struct notecard_usb_state {
	struct usb_device *udev;   /* USB device (Notecard) */
	uint8_t ctrl_iface;        /* CDC control interface number */
	uint8_t data_iface;        /* CDC data interface number */
	uint8_t ep_in;             /* Bulk IN endpoint (device → host) */
	uint8_t ep_out;            /* Bulk OUT endpoint (host → device) */
	bool    ready;             /* Device is connected and configured */
	struct k_sem tx_sync;      /* TX completion semaphore */
	struct k_sem connected;    /* Signals RX thread when device connects */
};

static struct notecard_usb_state usb_state;

RING_BUF_DECLARE(notecard_usb_rx_rb, CONFIG_BLUES_NOTECARD_USB_SERIAL_RX_BUF_SIZE);

/* RX thread */
K_THREAD_STACK_DEFINE(usb_rx_stack, CONFIG_BLUES_NOTECARD_USB_SERIAL_RX_THREAD_STACK_SIZE);
static struct k_thread usb_rx_thread;
static struct k_sem usb_rx_sync;

/* --------------------------------------------------------------------------
 * Transfer completion callbacks
 * -------------------------------------------------------------------------- */

static int tx_complete_cb(struct usb_device *udev, struct uhc_transfer *xfer)
{
	ARG_UNUSED(udev);

	if (xfer->err) {
		LOG_ERR("TX transfer error: %d", xfer->err);
	}

	/* Free resources before signalling -- caller does not need the xfer */
	if (xfer->buf) {
		usbh_xfer_buf_free(udev, xfer->buf);
	}
	usbh_xfer_free(udev, xfer);

	k_sem_give(&usb_state.tx_sync);
	return 0;
}

static int rx_complete_cb(struct usb_device *udev, struct uhc_transfer *xfer)
{
	ARG_UNUSED(udev);

	if (xfer->err == 0 && xfer->buf && xfer->buf->len > 0) {
		uint32_t put = ring_buf_put(&notecard_usb_rx_rb,
					    xfer->buf->data,
					    xfer->buf->len);
		if (put < xfer->buf->len) {
			LOG_WRN("RX ring buffer full, dropped %u bytes",
				xfer->buf->len - put);
		}
	} else if (xfer->err && xfer->err != -ECONNRESET) {
		LOG_ERR("RX transfer error: %d", xfer->err);
	}

	if (xfer->buf) {
		usbh_xfer_buf_free(udev, xfer->buf);
	}
	usbh_xfer_free(udev, xfer);

	k_sem_give(&usb_rx_sync);
	return 0;
}

static int ctrl_complete_cb(struct usb_device *udev, struct uhc_transfer *xfer)
{
	ARG_UNUSED(udev);

	if (xfer->err) {
		LOG_ERR("Control transfer error: %d", xfer->err);
	}
	/* Resources freed by the caller after waiting on the semaphore */
	k_sem_give(&usb_state.tx_sync);
	return 0;
}

/* --------------------------------------------------------------------------
 * CDC-ACM control request helpers
 * -------------------------------------------------------------------------- */

static int notecard_usb_set_line_coding(struct usb_device *udev,
					uint8_t ctrl_iface,
					uint32_t baud_rate)
{
	struct cdc_line_coding coding = {
		.dwDTERate   = sys_cpu_to_le32(baud_rate),
		.bCharFormat = 0, /* 1 stop bit */
		.bParityType = 0, /* No parity */
		.bDataBits   = 8,
	};
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(udev, sizeof(coding));
	if (!buf) {
		return -ENOMEM;
	}
	net_buf_add_mem(buf, &coding, sizeof(coding));

	/* Control OUT transfer (host to device) uses EP 0x00 */
	xfer = usbh_xfer_alloc(udev, 0x00u, ctrl_complete_cb, NULL);
	if (!xfer) {
		usbh_xfer_buf_free(udev, buf);
		return -ENOMEM;
	}

	/* Build SET_LINE_CODING setup packet */
	struct usb_setup_packet setup = {
		.bmRequestType = CDC_REQTYPE_HOST_TO_IF,
		.bRequest      = CDC_REQ_SET_LINE_CODING,
		.wValue        = 0,
		.wIndex        = sys_cpu_to_le16(ctrl_iface),
		.wLength       = sys_cpu_to_le16(sizeof(coding)),
	};
	memcpy(xfer->setup_pkt, &setup, sizeof(setup));

	ret = usbh_xfer_buf_add(udev, xfer, buf);
	if (ret) {
		usbh_xfer_buf_free(udev, buf);
		usbh_xfer_free(udev, xfer);
		return ret;
	}

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret) {
		usbh_xfer_buf_free(udev, buf);
		usbh_xfer_free(udev, xfer);
		return ret;
	}

	/* Wait for completion (ctrl_complete_cb signals tx_sync) */
	ret = k_sem_take(&usb_state.tx_sync, K_MSEC(5000));
	if (ret == 0 && xfer->err) {
		ret = xfer->err;
	}

	/* Free resources -- they are NOT freed in ctrl_complete_cb */
	if (xfer->buf) {
		usbh_xfer_buf_free(udev, xfer->buf);
	}
	usbh_xfer_free(udev, xfer);

	return ret;
}

static int notecard_usb_set_control_line_state(struct usb_device *udev,
					       uint8_t ctrl_iface,
					       bool dtr, bool rts)
{
	struct uhc_transfer *xfer;
	int ret;
	uint16_t value = (dtr ? BIT(0) : 0) | (rts ? BIT(1) : 0);

	/* Control OUT transfer with no data uses EP 0x00 */
	xfer = usbh_xfer_alloc(udev, 0x00u, ctrl_complete_cb, NULL);
	if (!xfer) {
		return -ENOMEM;
	}

	struct usb_setup_packet setup = {
		.bmRequestType = CDC_REQTYPE_HOST_TO_IF,
		.bRequest      = CDC_REQ_SET_CTRL_LINE_STATE,
		.wValue        = sys_cpu_to_le16(value),
		.wIndex        = sys_cpu_to_le16(ctrl_iface),
		.wLength       = 0,
	};
	memcpy(xfer->setup_pkt, &setup, sizeof(setup));

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret) {
		usbh_xfer_free(udev, xfer);
		return ret;
	}

	ret = k_sem_take(&usb_state.tx_sync, K_MSEC(5000));
	if (ret == 0 && xfer->err) {
		ret = xfer->err;
	}

	usbh_xfer_free(udev, xfer);
	return ret;
}

/* --------------------------------------------------------------------------
 * Note-C serial transport callbacks
 * -------------------------------------------------------------------------- */

static bool note_serial_reset(void)
{
	if (!usb_state.ready) {
		return false;
	}

	ring_buf_reset(&notecard_usb_rx_rb);
	return true;
}

static void note_serial_transmit(uint8_t *data, size_t len, bool flush)
{
	ARG_UNUSED(flush);

	if (!usb_state.ready || len == 0) {
		return;
	}

	struct usb_device *udev = usb_state.udev;
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(udev, len);
	if (!buf) {
		LOG_ERR("TX: failed to allocate buffer");
		return;
	}
	net_buf_add_mem(buf, data, len);

	xfer = usbh_xfer_alloc(udev, usb_state.ep_out, tx_complete_cb, NULL);
	if (!xfer) {
		LOG_ERR("TX: failed to allocate transfer");
		usbh_xfer_buf_free(udev, buf);
		return;
	}

	ret = usbh_xfer_buf_add(udev, xfer, buf);
	if (ret) {
		LOG_ERR("TX: failed to add buffer: %d", ret);
		usbh_xfer_buf_free(udev, buf);
		usbh_xfer_free(udev, xfer);
		return;
	}

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret) {
		LOG_ERR("TX: failed to enqueue: %d", ret);
		/* tx_complete_cb frees buf+xfer; we should not double-free.
		 * However if enqueue fails, the transfer was not submitted, so
		 * the callback will not be called and we must free manually.
		 */
		usbh_xfer_buf_free(udev, xfer->buf);
		usbh_xfer_free(udev, xfer);
		return;
	}

	/* Wait for TX completion (tx_complete_cb frees buf+xfer and signals) */
	ret = k_sem_take(&usb_state.tx_sync, K_MSEC(5000));
	if (ret) {
		LOG_ERR("TX: timeout waiting for completion");
	}
}

static bool note_serial_available(void)
{
	return !ring_buf_is_empty(&notecard_usb_rx_rb);
}

static char note_serial_receive(void)
{
	uint8_t c = 0;

	if (!ring_buf_is_empty(&notecard_usb_rx_rb)) {
		ring_buf_get(&notecard_usb_rx_rb, &c, 1);
	}

	return (char)c;
}

/* --------------------------------------------------------------------------
 * RX background thread
 * -------------------------------------------------------------------------- */

static void usb_rx_thread_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (true) {
		/* Block until a Notecard connects */
		k_sem_take(&usb_state.connected, K_FOREVER);

		LOG_DBG("RX thread: Notecard connected, starting RX loop");

		while (usb_state.ready) {
			struct usb_device *udev = usb_state.udev;
			struct uhc_transfer *xfer;
			struct net_buf *buf;
			int ret;

			buf = usbh_xfer_buf_alloc(udev, USB_SERIAL_BULK_BUF_SIZE);
			if (!buf) {
				LOG_ERR("RX: failed to allocate buffer");
				k_sleep(K_MSEC(10));
				continue;
			}

			/* Bulk IN transfer: Notecard → host */
			xfer = usbh_xfer_alloc(udev, usb_state.ep_in,
					       rx_complete_cb, NULL);
			if (!xfer) {
				LOG_ERR("RX: failed to allocate transfer");
				usbh_xfer_buf_free(udev, buf);
				k_sleep(K_MSEC(10));
				continue;
			}

			ret = usbh_xfer_buf_add(udev, xfer, buf);
			if (ret) {
				LOG_ERR("RX: failed to add buffer: %d", ret);
				usbh_xfer_buf_free(udev, buf);
				usbh_xfer_free(udev, xfer);
				k_sleep(K_MSEC(10));
				continue;
			}

			ret = usbh_xfer_enqueue(udev, xfer);
			if (ret) {
				LOG_ERR("RX: failed to enqueue: %d", ret);
				usbh_xfer_buf_free(udev, xfer->buf);
				usbh_xfer_free(udev, xfer);
				k_sleep(K_MSEC(10));
				continue;
			}

			/* Wait for RX completion; rx_complete_cb frees buf+xfer */
			k_sem_take(&usb_rx_sync, K_FOREVER);
		}

		LOG_DBG("RX thread: Notecard disconnected, waiting for reconnect");
	}
}

/* --------------------------------------------------------------------------
 * USBH CDC-ACM class driver
 * -------------------------------------------------------------------------- */

/**
 * Parse the CDC-ACM interface descriptors to find the data interface
 * and its bulk endpoints.
 *
 * For a standard CDC-ACM device the layout is:
 *   Interface N   (CDC control, class=0x02, subclass=0x02)
 *     [CDC functional descriptors]
 *     Endpoint: Interrupt IN (notifications)
 *   Interface N+1 (CDC data,    class=0x0A, subclass=0x00)
 *     Endpoint: Bulk OUT (host → device)
 *     Endpoint: Bulk IN  (device → host)
 */
static int find_cdc_data_endpoints(struct usb_device *udev,
				   uint8_t ctrl_iface,
				   uint8_t *data_iface_out,
				   uint8_t *ep_in_out,
				   uint8_t *ep_out_out)
{
	uint8_t data_iface_num = ctrl_iface + 1;
	uint8_t ep_in = 0;
	uint8_t ep_out = 0;

	/* Get the CDC data interface descriptor */
	const struct usb_if_descriptor *data_iface =
		usbh_desc_get_iface(udev, data_iface_num);

	if (!data_iface) {
		LOG_ERR("CDC data interface %u not found", data_iface_num);
		return -ENOENT;
	}

	if (data_iface->bInterfaceClass != USB_CDC_DATA_CLASS) {
		LOG_ERR("Interface %u class 0x%02x is not CDC data (0x%02x)",
			data_iface_num, data_iface->bInterfaceClass,
			USB_CDC_DATA_CLASS);
		return -EINVAL;
	}

	/* Iterate descriptors following the data interface header to find
	 * the bulk IN and bulk OUT endpoints.
	 */
	const struct usb_desc_header *desc =
		(const struct usb_desc_header *)usbh_desc_get_next(data_iface);

	while (desc) {
		if (desc->bDescriptorType == USB_DESC_INTERFACE) {
			/* Reached the next interface -- stop searching */
			break;
		}

		if (desc->bDescriptorType == USB_DESC_ENDPOINT) {
			const struct usb_ep_descriptor *ep =
				(const struct usb_ep_descriptor *)desc;

			bool is_bulk = ((ep->bmAttributes & USB_EP_TYPE_MASK)
					== USB_EP_TYPE_BULK);
			bool is_in   = (ep->bEndpointAddress & USB_EP_DIR_MASK)
					== USB_EP_DIR_IN;

			if (is_bulk && is_in) {
				ep_in = ep->bEndpointAddress;
			} else if (is_bulk && !is_in) {
				ep_out = ep->bEndpointAddress;
			}
		}

		desc = usbh_desc_get_next(desc);
	}

	if (!ep_in || !ep_out) {
		LOG_ERR("CDC data interface missing bulk endpoints "
			"(ep_in=0x%02x, ep_out=0x%02x)", ep_in, ep_out);
		return -ENOENT;
	}

	*data_iface_out = data_iface_num;
	*ep_in_out      = ep_in;
	*ep_out_out     = ep_out;

	return 0;
}

static int notecard_usb_probe(struct usbh_class_data *const c_data,
			      struct usb_device *const udev,
			      const uint8_t iface)
{
	int ret;

	/* Check that this interface is a CDC ACM control interface */
	const struct usb_if_descriptor *ctrl_iface =
		usbh_desc_get_iface(udev, iface);

	if (!ctrl_iface) {
		return -ENOTSUP;
	}

	if (ctrl_iface->bInterfaceClass    != USB_CDC_CLASS ||
	    ctrl_iface->bInterfaceSubClass != USB_CDC_ACM_SUBCLASS) {
		return -ENOTSUP;
	}

	LOG_INF("Notecard USB serial: found CDC-ACM control interface %u", iface);

	/* Find the associated data interface and its bulk endpoints */
	uint8_t data_iface, ep_in, ep_out;

	ret = find_cdc_data_endpoints(udev, iface, &data_iface, &ep_in, &ep_out);
	if (ret) {
		return -ENOTSUP;
	}

	LOG_INF("Notecard USB serial: data_iface=%u ep_in=0x%02x ep_out=0x%02x",
		data_iface, ep_in, ep_out);

	/* Claim the interface */
	c_data->udev  = udev;
	c_data->iface = iface;

	/* Store transport state */
	usb_state.udev       = udev;
	usb_state.ctrl_iface = iface;
	usb_state.data_iface = data_iface;
	usb_state.ep_in      = ep_in;
	usb_state.ep_out     = ep_out;

	/* Configure CDC-ACM line coding.
	 * For USB CDC-ACM the "baud rate" is a virtual parameter that does not
	 * affect actual USB throughput, but setting it to a high value signals
	 * intent to the device firmware.
	 */
	ret = notecard_usb_set_line_coding(udev, iface, 115200);
	if (ret) {
		LOG_WRN("SET_LINE_CODING failed: %d (continuing)", ret);
	}

	/* Assert DTR + RTS to signal host is ready */
	ret = notecard_usb_set_control_line_state(udev, iface, true, true);
	if (ret) {
		LOG_WRN("SET_CONTROL_LINE_STATE failed: %d (continuing)", ret);
	}

	/* Mark as ready and wake the RX thread */
	usb_state.ready = true;
	k_sem_give(&usb_state.connected);

	LOG_INF("Notecard USB serial: transport ready");
	return 0;
}

static int notecard_usb_removed(struct usbh_class_data *const c_data)
{
	ARG_UNUSED(c_data);

	LOG_INF("Notecard USB serial: device removed");

	usb_state.ready = false;
	usb_state.udev  = NULL;

	ring_buf_reset(&notecard_usb_rx_rb);

	/* Unblock any pending TX/RX */
	k_sem_give(&usb_state.tx_sync);
	k_sem_give(&usb_rx_sync);

	return 0;
}

static int notecard_usb_init(struct usbh_class_data *const c_data)
{
	ARG_UNUSED(c_data);
	return 0;
}

static struct usbh_class_api notecard_usb_class_api = {
	.init          = notecard_usb_init,
	.probe         = notecard_usb_probe,
	.removed       = notecard_usb_removed,
};

/* Match any CDC-ACM interface (class=0x02, subclass=0x02, any protocol).
 * The filter array must be terminated by an entry with flags=0.
 */
static const struct usbh_class_filter notecard_usb_filters[] = {
	{
		.class = USB_CDC_CLASS,
		.sub   = USB_CDC_ACM_SUBCLASS,
		.proto = 0x01, /* Hayes/V.25ter protocol */
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
	},
	{
		.class = USB_CDC_CLASS,
		.sub   = USB_CDC_ACM_SUBCLASS,
		.proto = 0x00, /* No specific protocol */
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
	},
	{ .flags = 0 }, /* Terminator */
};

USBH_DEFINE_CLASS(notecard_usb_serial,
		  &notecard_usb_class_api,
		  NULL,
		  notecard_usb_filters);

/* --------------------------------------------------------------------------
 * System initialization
 * -------------------------------------------------------------------------- */

static int notecard_usb_serial_init(void)
{
	int ret;

	k_sem_init(&usb_state.tx_sync, 0, 1);
	k_sem_init(&usb_state.connected, 0, 1);
	k_sem_init(&usb_rx_sync, 0, 1);

	/* Register note-c serial callbacks.  The callbacks handle the
	 * "not connected" case gracefully until a Notecard is detected.
	 */
	NoteSetFnSerial(note_serial_reset,
			note_serial_transmit,
			note_serial_available,
			note_serial_receive);

	/* Initialize and enable each USB host controller defined by the
	 * application.  Typically there is exactly one, defined via
	 * USBH_CONTROLLER_DEFINE().
	 */
	STRUCT_SECTION_FOREACH(usbh_context, uhs_ctx) {
		ret = usbh_init(uhs_ctx);
		if (ret) {
			LOG_ERR("Failed to init USB host controller: %d", ret);
			return ret;
		}

		ret = usbh_enable(uhs_ctx);
		if (ret) {
			LOG_ERR("Failed to enable USB host controller: %d", ret);
			return ret;
		}

		/* Only initialize the first controller */
		break;
	}

	/* Start the background RX thread */
	k_thread_create(&usb_rx_thread, usb_rx_stack,
			K_THREAD_STACK_SIZEOF(usb_rx_stack),
			usb_rx_thread_fn, NULL, NULL, NULL,
			CONFIG_BLUES_NOTECARD_USB_SERIAL_RX_THREAD_PRIO,
			0, K_NO_WAIT);
	k_thread_name_set(&usb_rx_thread, "notecard_usb_rx");

	LOG_INF("Notecard USB serial transport initialized, waiting for device");
	return 0;
}

SYS_INIT(notecard_usb_serial_init,
	 APPLICATION,
	 CONFIG_APPLICATION_INIT_PRIORITY);
