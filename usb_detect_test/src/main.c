#include <zephyr/usb/usb_device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(usb_status, LOG_LEVEL_INF);

static void usb_status_callback(enum usb_dc_status_code status, const uint8_t *param)
{
    switch (status) {
    case USB_DC_CONNECTED:
        LOG_INF("USB connected to host");
        break;
    case USB_DC_DISCONNECTED:
        LOG_INF("USB disconnected from host");
        break;
    case USB_DC_SUSPEND:
        LOG_INF("USB suspended");
        break;
    case USB_DC_RESUME:
        LOG_INF("USB resumed");
        break;
    default:
        break;
    }
}

void main(void)
{
    int ret;

    ret = usb_enable(usb_status_callback);
    if (ret != 0) {
        LOG_ERR("Failed to enable USB");
        return;
    }
}