/*
 * Dual Mode HID Mouse - USB and BLE
 * BLE implementation based on working BLE_HID_mouse_main.c
 * Adds debounced mode switching to avoid rapid USB↔BLE flips
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zephyr/input/input.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <bluetooth/services/hids.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/services/dis.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/sys/byteorder.h>
#include <dk_buttons_and_leds.h>


#include <stdbool.h>  /* if not already present */  /* [attached_file:3] */
static void num_comp_reply(bool accept);            /* [attached_file:3] */


LOG_MODULE_REGISTER(dual_hid, LOG_LEVEL_INF);

/* Constants from working BLE code */
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define BASE_USB_HID_SPEC_VERSION 0x0101
#define MOVEMENT_SPEED 5
#define INPUT_REPORT_COUNT 3
#define INPUT_REP_BUTTONS_LEN 3
#define INPUT_REP_MOVEMENT_LEN 3
#define INPUT_REP_MEDIA_PLAYER_LEN 1
#define INPUT_REP_BUTTONS_INDEX 0
#define INPUT_REP_MOVEMENT_INDEX 1
#define INPUT_REP_MPLAYER_INDEX 2
#define INPUT_REP_REF_BUTTONS_ID 1
#define INPUT_REP_REF_MOVEMENT_ID 2
#define INPUT_REP_REF_MPLAYER_ID 3
#define HIDS_QUEUE_SIZE 10

/* DK button masks */
#define KEY_LEFT_MASK DK_BTN1_MSK
#define KEY_UP_MASK DK_BTN2_MSK
#define KEY_RIGHT_MASK DK_BTN3_MSK
#define KEY_DOWN_MASK DK_BTN4_MSK
#define KEY_PAIRING_ACCEPT DK_BTN1_MSK
#define KEY_PAIRING_REJECT DK_BTN2_MSK

/* USB HID Report Descriptor (mouse) */
static const uint8_t usb_hid_report_desc[] = {
    0x05, 0x01,                    /* Usage Page (Generic Desktop) */
    0x09, 0x02,                    /* Usage (Mouse) */
    0xA1, 0x01,                    /* Collection (Application) */
    0x09, 0x01,                    /*   Usage (Pointer) */
    0xA1, 0x00,                    /*   Collection (Physical) */
    0x05, 0x09,                    /*     Usage Page (Button) */
    0x19, 0x01,                    /*     Usage Minimum (01) */
    0x29, 0x03,                    /*     Usage Maximum (03) */
    0x15, 0x00,                    /*     Logical Minimum (0) */
    0x25, 0x01,                    /*     Logical Maximum (1) */
    0x95, 0x03,                    /*     Report Count (3) */
    0x75, 0x01,                    /*     Report Size (1) */
    0x81, 0x02,                    /*     Input (Data,Var,Abs) */
    0x95, 0x01,                    /*     Report Count (1) */
    0x75, 0x05,                    /*     Report Size (5) */
    0x81, 0x03,                    /*     Input (Const,Var,Abs) */
    0x05, 0x01,                    /*     Usage Page (Generic Desktop) */
    0x09, 0x30,                    /*     Usage (X) */
    0x09, 0x31,                    /*     Usage (Y) */
    0x09, 0x38,                    /*     Usage (Wheel) */
    0x15, 0x81,                    /*     Logical Minimum (-127) */
    0x25, 0x7F,                    /*     Logical Maximum (127) */
    0x75, 0x08,                    /*     Report Size (8) */
    0x95, 0x03,                    /*     Report Count (3) */
    0x81, 0x06,                    /*     Input (Data,Var,Rel) */
    0xC0,                          /*   End Collection */
    0xC0                           /* End Collection */
};

/* Modes */
typedef enum {
    HID_MODE_NONE,
    HID_MODE_USB,
    HID_MODE_BLE
} hid_mode_t;

/* Debounced desired mode */
enum desired_mode_e { DESIRED_NONE, DESIRED_USB, DESIRED_BLE };

/* USB report */
struct usb_mouse_report {
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t wheel;
};

/* BLE mouse pos (from working code) */
struct mouse_pos {
    int16_t x_val;
    int16_t y_val;
};

/* Global state */
static hid_mode_t current_mode = HID_MODE_NONE;
static enum desired_mode_e desired_mode = DESIRED_NONE;
static struct k_work_delayable mode_work;
static bool ble_link_active = false;
static bool usb_connected = false;
static bool usb_configured = false;

/* USB HID */
static const struct device *usb_hid_dev;
static enum usb_dc_status_code usb_status;
K_MSGQ_DEFINE(usb_msgq, sizeof(struct usb_mouse_report), 10, 1);
static K_SEM_DEFINE(usb_ep_write_sem, 0, 1);

/* BLE HID (from working code) */
BT_HIDS_DEF(hids_obj,
            INPUT_REP_BUTTONS_LEN,
            INPUT_REP_MOVEMENT_LEN,
            INPUT_REP_MEDIA_PLAYER_LEN);

static struct k_work hids_work;
static bool ble_ready = false;

/* Movement queue */
K_MSGQ_DEFINE(hids_queue,
              sizeof(struct mouse_pos),
              HIDS_QUEUE_SIZE,
              4);

/* Optional directed adv queue */
#if CONFIG_BT_DIRECTED_ADVERTISING
K_MSGQ_DEFINE(bonds_queue,
              sizeof(bt_addr_le_t),
              CONFIG_BT_MAX_PAIRED,
              4);
#endif

/* Connection slots */
static struct conn_mode {
    struct bt_conn *conn;
    bool in_boot_mode;
} conn_mode[CONFIG_BT_HIDS_MAX_CLIENT_COUNT];

/* Adv and pairing works */
static volatile bool is_adv_running;
static struct k_work adv_work;
static struct k_work pairing_work;

/* MITM queue */
struct pairing_data_mitm {
    struct bt_conn *conn;
    unsigned int passkey;
};

K_MSGQ_DEFINE(mitm_queue,
              sizeof(struct pairing_data_mitm),
              CONFIG_BT_HIDS_MAX_CLIENT_COUNT,
              4);

/* Advertising data (from working code) */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
                  (CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
                  (CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                  BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
                  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
};

static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

/* Helpers */
static bool any_ble_connected(void)
{
    for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++) {
        if (conn_mode[i].conn) {
            return true;
        }
    }
    return false;
}

/* Forward decls */
static void switch_to_usb_mode(void);
static void switch_to_ble_mode(void);

/* ========================= USB HID ========================= */

static void usb_int_in_ready_cb(const struct device *dev)
{
    ARG_UNUSED(dev);
    k_sem_give(&usb_ep_write_sem);
}

static const struct hid_ops usb_hid_ops = {
    .int_in_ready = usb_int_in_ready_cb,
};

static void usb_status_callback(enum usb_dc_status_code status, const uint8_t *param)
{
    usb_status = status;
    switch (status) {
    case USB_DC_CONNECTED:
        LOG_INF("USB connected to host");
        usb_connected = true;
        break;
    case USB_DC_DISCONNECTED:
        LOG_INF("USB disconnected from host");
        usb_connected = false;
        usb_configured = false;
        desired_mode = DESIRED_BLE;
        k_work_reschedule(&mode_work, K_MSEC(1500));
        break;
    case USB_DC_CONFIGURED:
        LOG_INF("USB device configured");
        usb_configured = true;
        desired_mode = DESIRED_USB;
        k_work_reschedule(&mode_work, K_MSEC(1500));
        break;
    case USB_DC_SUSPEND:
        LOG_INF("USB suspended");
        desired_mode = DESIRED_BLE;
        k_work_reschedule(&mode_work, K_MSEC(1500));
        break;
    case USB_DC_RESUME:
        LOG_INF("USB resumed");
        break;
    default:
        break;
    }
}

static int init_usb_hid(void)
{
    int ret;

    usb_hid_dev = device_get_binding("HID_0");
    if (!usb_hid_dev) {
        LOG_ERR("USB HID device not found");
        return -ENODEV;
    }

    usb_hid_register_device(usb_hid_dev, usb_hid_report_desc,
                            sizeof(usb_hid_report_desc), &usb_hid_ops);

    ret = usb_hid_init(usb_hid_dev);
    if (ret != 0) {
        LOG_ERR("Failed to init USB HID: %d", ret);
        return ret;
    }

    return 0;
}

static void usb_hid_worker(void)
{
    struct usb_mouse_report report;

    while (true) {
        if (current_mode == HID_MODE_USB && usb_configured) {
            if (k_msgq_get(&usb_msgq, &report, K_MSEC(100)) == 0) {
                int ret = hid_int_ep_write(usb_hid_dev, (uint8_t *)&report,
                                           sizeof(report), NULL);
                if (ret == 0) {
                    k_sem_take(&usb_ep_write_sem, K_FOREVER);
                }
            }
        } else {
            k_sleep(K_MSEC(100));
        }
    }
}

K_THREAD_DEFINE(usb_hid_thread, 1024, usb_hid_worker, NULL, NULL, NULL, 5, 0, 0);

/* ========================= BLE HID (from working code) ========================= */

#if CONFIG_BT_DIRECTED_ADVERTISING
static void bond_find(const struct bt_bond_info *info, void *user_data)
{
    int err;

    for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++) {
        if (conn_mode[i].conn) {
            const bt_addr_le_t *dst = bt_conn_get_dst(conn_mode[i].conn);
            if (!bt_addr_le_cmp(&info->addr, dst)) {
                return;
            }
        }
    }

    err = k_msgq_put(&bonds_queue, (void *)&info->addr, K_NO_WAIT);
    if (err) {
        printk("No space in the queue for the bond.\n");
    }
}
#endif

static void advertising_continue(void)
{
    struct bt_le_adv_param adv_param;

#if CONFIG_BT_DIRECTED_ADVERTISING
    bt_addr_le_t addr;

    if (!k_msgq_get(&bonds_queue, &addr, K_NO_WAIT)) {
        char addr_buf[BT_ADDR_LE_STR_LEN];
        int err;

        if (is_adv_running) {
            err = bt_le_adv_stop();
            if (err) {
                printk("Advertising failed to stop (err %d)\n", err);
                return;
            }
            is_adv_running = false;
        }

        adv_param = *BT_LE_ADV_CONN_DIR(&addr);
        adv_param.options |= BT_LE_ADV_OPT_DIR_ADDR_RPA;

        err = bt_le_adv_start(&adv_param, NULL, 0, NULL, 0);
        if (err) {
            printk("Directed advertising failed to start (err %d)\n", err);
            return;
        }

        bt_addr_le_to_str(&addr, addr_buf, BT_ADDR_LE_STR_LEN);
        printk("Direct advertising to %s started\n", addr_buf);
    } else
#endif
    {
        int err;

        if (is_adv_running) {
            return;
        }

        adv_param = *BT_LE_ADV_CONN_FAST_2;
        err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad),
                              sd, ARRAY_SIZE(sd));
        if (err) {
            printk("Advertising failed to start (err %d)\n", err);
            return;
        }

        printk("Regular advertising started\n");
    }

    is_adv_running = true;
}

static void advertising_start(void)
{
#if CONFIG_BT_DIRECTED_ADVERTISING
    k_msgq_purge(&bonds_queue);
    bt_foreach_bond(BT_ID_DEFAULT, bond_find, NULL);
#endif

    k_work_submit(&adv_work);
}

static void advertising_process(struct k_work *work)
{
    advertising_continue();
}

static void pairing_process(struct k_work *work)
{
    int err;
    struct pairing_data_mitm pairing_data;
    char addr[BT_ADDR_LE_STR_LEN];

    err = k_msgq_peek(&mitm_queue, &pairing_data);
    if (err) {
        return;
    }

    bt_addr_le_to_str(bt_conn_get_dst(pairing_data.conn),
                      addr, sizeof(addr));
    printk("Passkey for %s: %06u\n", addr, pairing_data.passkey);

    if (IS_ENABLED(CONFIG_SOC_SERIES_NRF54HX) || IS_ENABLED(CONFIG_SOC_SERIES_NRF54LX)) {
        printk("Press Button 0 to confirm, Button 1 to reject.\n");
    } else {
        printk("Press Button 1 to confirm, Button 2 to reject.\n");
    }
}

static void insert_conn_object(struct bt_conn *conn)
{
    for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++) {
        if (!conn_mode[i].conn) {
            conn_mode[i].conn = conn;
            conn_mode[i].in_boot_mode = false;
            return;
        }
    }
    printk("Connection object could not be inserted %p\n", conn);
}

static void remove_conn_object(struct bt_conn *conn)
{
    for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++) {
        if (conn_mode[i].conn == conn) {
            conn_mode[i].conn = NULL;
            conn_mode[i].in_boot_mode = false;
            return;
        }
    }
}

static void ble_connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    is_adv_running = false;
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
        if (err == BT_HCI_ERR_ADV_TIMEOUT) {
            printk("Direct advertising to %s timed out\n", addr);
            k_work_submit(&adv_work);
        } else {
            printk("Failed to connect to %s 0x%02x %s\n",
                   addr, err, bt_hci_err_to_str(err));
        }
        return;
    }

    printk("Connected %s\n", addr);

    err = bt_hids_connected(&hids_obj, conn);
    if (err) {
        printk("Failed to notify HID service about connection\n");
        return;
    }

    insert_conn_object(conn);
    ble_link_active = true;

    /* IMPORTANT: Do NOT re-advertise on connect; keep single-client stable */
    /* if (is_conn_slot_free()) { advertising_start(); } */
}

static void ble_disconnected(struct bt_conn *conn, uint8_t reason)
{
    int err;
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    printk("Disconnected from %s, reason 0x%02x %s\n",
           addr, reason, bt_hci_err_to_str(reason));

    err = bt_hids_disconnected(&hids_obj, conn);
    if (err) {
        printk("Failed to notify HID service about disconnection\n");
    }

    remove_conn_object(conn);
    ble_link_active = any_ble_connected();

    if (current_mode == HID_MODE_BLE && !usb_connected) {
        advertising_start();
    }
}

#ifdef CONFIG_BT_HIDS_SECURITY_ENABLED
static void security_changed(struct bt_conn *conn, bt_security_t level,
                             enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (!err) {
        printk("Security changed: %s level %u\n", addr, level);
    } else {
        printk("Security failed: %s level %u err %d %s\n",
               addr, level, err, bt_security_err_to_str(err));
    }
}
#endif

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = ble_connected,
    .disconnected = ble_disconnected,
#ifdef CONFIG_BT_HIDS_SECURITY_ENABLED
    .security_changed = security_changed,
#endif
};

static void hids_pm_evt_handler(enum bt_hids_pm_evt evt, struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];
    size_t i;

    for (i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++) {
        if (conn_mode[i].conn == conn) {
            break;
        }
    }

    if (i >= CONFIG_BT_HIDS_MAX_CLIENT_COUNT) {
        return;
    }

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    switch (evt) {
    case BT_HIDS_PM_EVT_BOOT_MODE_ENTERED:
        printk("Boot mode entered %s\n", addr);
        conn_mode[i].in_boot_mode = true;
        break;

    case BT_HIDS_PM_EVT_REPORT_MODE_ENTERED:
        printk("Report mode entered %s\n", addr);
        conn_mode[i].in_boot_mode = false;
        break;

    default:
        break;
    }
}

static void hid_init(void)
{
    int err;
    struct bt_hids_init_param hids_init_param = { 0 };
    struct bt_hids_inp_rep *hids_inp_rep;

    static const uint8_t mouse_movement_mask[DIV_ROUND_UP(INPUT_REP_MOVEMENT_LEN, 8)] = {0};

    /* Report map from working BLE code */
    static const uint8_t report_map[] = {
        0x05, 0x01,                    /* Usage Page (Generic Desktop) */
        0x09, 0x02,                    /* Usage (Mouse) */
        0xA1, 0x01,                    /* Collection (Application) */

        /* Report ID 1: Buttons + scroll/pan */
        0x85, 0x01,                    /*   Report Id 1 */
        0x09, 0x01,                    /*   Usage (Pointer) */
        0xA1, 0x00,                    /*   Collection (Physical) */
        0x95, 0x05,                    /*     Report Count (5 buttons) */
        0x75, 0x01,                    /*     Report Size (1) */
        0x05, 0x09,                    /*     Usage Page (Buttons) */
        0x19, 0x01,                    /*     Usage Minimum (01) */
        0x29, 0x05,                    /*     Usage Maximum (05) */
        0x15, 0x00,                    /*     Logical Minimum (0) */
        0x25, 0x01,                    /*     Logical Maximum (1) */
        0x81, 0x02,                    /*     Input (Data,Var,Abs) */
        0x95, 0x01,                    /*     Report Count (1) */
        0x75, 0x03,                    /*     Report Size (3) */
        0x81, 0x01,                    /*     Input (Const,Array,Abs) */
        0x75, 0x08,                    /*     Report Size (8) */
        0x95, 0x01,                    /*     Report Count (1) */
        0x05, 0x01,                    /*     Usage Page (Generic Desktop) */
        0x09, 0x38,                    /*     Usage (Wheel) */
        0x15, 0x81,                    /*     Logical Minimum (-127) */
        0x25, 0x7F,                    /*     Logical Maximum (127) */
        0x81, 0x06,                    /*     Input (Data,Var,Rel) */
        0x05, 0x0C,                    /*     Usage Page (Consumer) */
        0x0A, 0x38, 0x02,              /*     Usage (AC Pan) */
        0x95, 0x01,                    /*     Report Count (1) */
        0x81, 0x06,                    /*     Input (Data,Var,Rel) */
        0xC0,                          /*   End Collection (Physical) */

        /* Report ID 2: Motion (12-bit per axis) */
        0x85, 0x02,                    /*   Report Id 2 */
        0x09, 0x01,                    /*   Usage (Pointer) */
        0xA1, 0x00,                    /*   Collection (Physical) */
        0x75, 0x0C,                    /*     Report Size (12) */
        0x95, 0x02,                    /*     Report Count (2) */
        0x05, 0x01,                    /*     Usage Page (Generic Desktop) */
        0x09, 0x30,                    /*     Usage (X) */
        0x09, 0x31,                    /*     Usage (Y) */
        0x16, 0x01, 0xF8,              /*     Logical Min (-2047) */
        0x26, 0xFF, 0x07,              /*     Logical Max (2047) */
        0x81, 0x06,                    /*     Input (Data,Var,Rel) */
        0xC0,                          /*   End Collection (Physical) */
        0xC0,                          /* End Collection (Application) */

        /* Report ID 3: Consumer controls */
        0x05, 0x0C,                    /* Usage Page (Consumer) */
        0x09, 0x01,                    /* Usage (Consumer Control) */
        0xA1, 0x01,                    /* Collection (Application) */
        0x85, 0x03,                    /*   Report Id (3) */
        0x15, 0x00,                    /*   Logical Minimum (0) */
        0x25, 0x01,                    /*   Logical Maximum (1) */
        0x75, 0x01,                    /*   Report Size (1) */
        0x95, 0x01,                    /*   Report Count (1) */
        0x09, 0xCD,                    /*   Usage (Play/Pause) */
        0x81, 0x06,                    /*   Input (Data,Var,Rel) */
        0x0A, 0x83, 0x01,              /*   Usage (Consumer Control Configuration) */
        0x81, 0x06,                    /*   Input (Data,Var,Rel) */
        0x09, 0xB5,                    /*   Usage (Next Track) */
        0x81, 0x06,                    /*   Input (Data,Var,Rel) */
        0x09, 0xB6,                    /*   Usage (Prev Track) */
        0x81, 0x06,                    /*   Input (Data,Var,Rel) */
        0x09, 0xEA,                    /*   Usage (Volume Down) */
        0x81, 0x06,                    /*   Input (Data,Var,Rel) */
        0x09, 0xE9,                    /*   Usage (Volume Up) */
        0x81, 0x06,                    /*   Input (Data,Var,Rel) */
        0x0A, 0x25, 0x02,              /*   Usage (AC Forward) */
        0x81, 0x06,                    /*   Input (Data,Var,Rel) */
        0x0A, 0x24, 0x02,              /*   Usage (AC Back) */
        0x81, 0x06,                    /*   Input (Data,Var,Rel) */
        0xC0                           /* End Collection */
    };

    hids_init_param.rep_map.data = report_map;
    hids_init_param.rep_map.size = sizeof(report_map);

    hids_init_param.info.bcd_hid = BASE_USB_HID_SPEC_VERSION;
    hids_init_param.info.b_country_code = 0x00;
    hids_init_param.info.flags = (BT_HIDS_REMOTE_WAKE |
                                  BT_HIDS_NORMALLY_CONNECTABLE);

    struct bt_hids_inp_rep *hids_inp_rep_group = hids_init_param.inp_rep_group_init.reports;

    hids_inp_rep = &hids_inp_rep_group[0];
    hids_inp_rep->size = INPUT_REP_BUTTONS_LEN;
    hids_inp_rep->id = INPUT_REP_REF_BUTTONS_ID;
    hids_init_param.inp_rep_group_init.cnt++;

    hids_inp_rep = &hids_inp_rep_group[1];
    hids_inp_rep->size = INPUT_REP_MOVEMENT_LEN;
    hids_inp_rep->id = INPUT_REP_REF_MOVEMENT_ID;
    hids_inp_rep->rep_mask = mouse_movement_mask;
    hids_init_param.inp_rep_group_init.cnt++;

    hids_inp_rep = &hids_inp_rep_group[2];
    hids_inp_rep->size = INPUT_REP_MEDIA_PLAYER_LEN;
    hids_inp_rep->id = INPUT_REP_REF_MPLAYER_ID;
    hids_init_param.inp_rep_group_init.cnt++;

    hids_init_param.is_mouse = true;
    hids_init_param.pm_evt_handler = hids_pm_evt_handler;

    err = bt_hids_init(&hids_obj, &hids_init_param);
    __ASSERT(err == 0, "HIDS initialization failed\n");
}

static void mouse_movement_send(int16_t x_delta, int16_t y_delta)
{
    for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++) {
        if (!conn_mode[i].conn) {
            continue;
        }

        if (conn_mode[i].in_boot_mode) {
            x_delta = MAX(MIN(x_delta, SCHAR_MAX), SCHAR_MIN);
            y_delta = MAX(MIN(y_delta, SCHAR_MAX), SCHAR_MIN);

            bt_hids_boot_mouse_inp_rep_send(&hids_obj,
                                            conn_mode[i].conn,
                                            NULL,
                                            (int8_t)x_delta,
                                            (int8_t)y_delta,
                                            NULL);
        } else {
            uint8_t x_buff[2];
            uint8_t y_buff[2];
            uint8_t buffer[INPUT_REP_MOVEMENT_LEN];

            int16_t x = MAX(MIN(x_delta, 0x07ff), -0x07ff);
            int16_t y = MAX(MIN(y_delta, 0x07ff), -0x07ff);

            sys_put_le16(x, x_buff);
            sys_put_le16(y, y_buff);

            BUILD_ASSERT(sizeof(buffer) == 3,
                         "Only 2 axis, 12-bit each, are supported");

            buffer[0] = x_buff[0];
            buffer[1] = (y_buff[0] << 4) | (x_buff[1] & 0x0f);
            buffer[2] = (y_buff[1] << 4) | (y_buff[0] >> 4);

            bt_hids_inp_rep_send(&hids_obj, conn_mode[i].conn,
                                 INPUT_REP_MOVEMENT_INDEX,
                                 buffer, sizeof(buffer), NULL);
        }
    }
}

static void mouse_handler(struct k_work *work)
{
    struct mouse_pos pos;

    while (!k_msgq_get(&hids_queue, &pos, K_NO_WAIT)) {
        mouse_movement_send(pos.x_val, pos.y_val);
    }
}

/* Authentication (unchanged from working code) */
#if defined(CONFIG_BT_HIDS_SECURITY_ENABLED)
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
    int err;
    struct pairing_data_mitm pairing_data;

    pairing_data.conn = bt_conn_ref(conn);
    pairing_data.passkey = passkey;

    err = k_msgq_put(&mitm_queue, &pairing_data, K_NO_WAIT);
    if (err) {
        printk("Pairing queue is full. Purge previous data.\n");
    }

    if (k_msgq_num_used_get(&mitm_queue) == 1) {
        k_work_submit(&pairing_work);
    }
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    printk("Pairing cancelled: %s\n", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    struct pairing_data_mitm pairing_data;

    if (k_msgq_peek(&mitm_queue, &pairing_data) != 0) {
        return;
    }

    if (pairing_data.conn == conn) {
        bt_conn_unref(pairing_data.conn);
        k_msgq_get(&mitm_queue, &pairing_data, K_NO_WAIT);
    }

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    printk("Pairing failed conn: %s, reason %d %s\n",
           addr, reason, bt_security_err_to_str(reason));
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = auth_passkey_display,
    .passkey_confirm = auth_passkey_confirm,
    .cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
    .pairing_complete = pairing_complete,
    .pairing_failed = pairing_failed
};
#else
static struct bt_conn_auth_cb conn_auth_callbacks;
static struct bt_conn_auth_info_cb conn_auth_info_callbacks;
#endif /* CONFIG_BT_HIDS_SECURITY_ENABLED */

/* Must be outside of any #if so it always links */             /* [attached_file:3] */
static void num_comp_reply(bool accept)                          /* [attached_file:3] */
{
    struct pairing_data_mitm pairing_data;                       /* [attached_file:3] */
    struct bt_conn *conn;                                        /* [attached_file:3] */

    if (k_msgq_get(&mitm_queue, &pairing_data, K_NO_WAIT) != 0) {
        return;                                                  /* [attached_file:3] */
    }

    conn = pairing_data.conn;                                    /* [attached_file:3] */

    if (accept) {
        bt_conn_auth_passkey_confirm(conn);                      /* [attached_file:1] */
        printk("Numeric Match, conn %p\n", conn);                /* [attached_file:1] */
    } else {
        bt_conn_auth_cancel(conn);                               /* [attached_file:1] */
        printk("Numeric Reject, conn %p\n", conn);               /* [attached_file:1] */
    }

    bt_conn_unref(pairing_data.conn);                            /* [attached_file:1] */

    if (k_msgq_num_used_get(&mitm_queue)) {
        k_work_submit(&pairing_work);                            /* [attached_file:1] */
    }
}


/* ========================= Debounced mode worker ========================= */

static void mode_worker(struct k_work *work)
{
    ARG_UNUSED(work);

    if (desired_mode == DESIRED_USB) {
        if (!ble_link_active && !any_ble_connected()) {
            switch_to_usb_mode();
            desired_mode = DESIRED_NONE;
        } else {
            k_work_reschedule(&mode_work, K_MSEC(1500));
        }
        return;
    }

    if (desired_mode == DESIRED_BLE) {
        switch_to_ble_mode();
        desired_mode = DESIRED_NONE;
        return;
    }
}

/* ========================= Mode switching ========================= */

static void switch_to_usb_mode(void)
{
    if (current_mode == HID_MODE_USB) {
        return;
    }
    if (ble_link_active || any_ble_connected()) {
        desired_mode = DESIRED_USB;
        k_work_reschedule(&mode_work, K_MSEC(1500));
        return;
    }

    LOG_INF("Switching to USB HID mode");

    if (current_mode == HID_MODE_BLE) {
        bt_le_adv_stop();
        is_adv_running = false;
    }

    current_mode = HID_MODE_USB;
}

static void switch_to_ble_mode(void)
{
    if (current_mode == HID_MODE_BLE) {
        return;
    }

    LOG_INF("Switching to BLE HID mode");
    current_mode = HID_MODE_BLE;

    if (ble_ready) {
        advertising_start();
    }
}

/* ========================= Input handling ========================= */

void button_changed(uint32_t button_state, uint32_t has_changed)
{
    bool data_to_send = false;
    struct mouse_pos pos;
    struct usb_mouse_report usb_report = {0};

    memset(&pos, 0, sizeof(struct mouse_pos));

    /* First handle pairing confirm/reject on edge */
    if (IS_ENABLED(CONFIG_BT_HIDS_SECURITY_ENABLED) && k_msgq_num_used_get(&mitm_queue)) {

        /* Button 1 (confirm) edge */
        if (has_changed & KEY_PAIRING_ACCEPT) {
            /* Treat as pressed when bit is set in button_state */
            if (button_state & KEY_PAIRING_ACCEPT) {
                num_comp_reply(true);
                return;
            }
        }

        /* Button 2 (reject) edge */
        if (has_changed & KEY_PAIRING_REJECT) {
            if (button_state & KEY_PAIRING_REJECT) {
                num_comp_reply(false);
                return;
            }
        }
    }

    /* The rest (movement) can still use the simple edge mask if desired */
    uint32_t buttons = button_state & has_changed;

    if (buttons & KEY_LEFT_MASK)  { pos.x_val -= MOVEMENT_SPEED; usb_report.x = -MOVEMENT_SPEED; data_to_send = true; }
    if (buttons & KEY_UP_MASK)    { pos.y_val -= MOVEMENT_SPEED;  usb_report.y = -MOVEMENT_SPEED; data_to_send = true; }
    if (buttons & KEY_RIGHT_MASK) { pos.x_val += MOVEMENT_SPEED;  usb_report.x =  MOVEMENT_SPEED; data_to_send = true; }
    if (buttons & KEY_DOWN_MASK)  { pos.y_val += MOVEMENT_SPEED;  usb_report.y =  MOVEMENT_SPEED; data_to_send = true; }

    if (data_to_send) {
        switch (current_mode) {
        case HID_MODE_USB:
            if (usb_configured) { k_msgq_put(&usb_msgq, &usb_report, K_NO_WAIT); }
            break;
        case HID_MODE_BLE:
            k_msgq_put(&hids_queue, &pos, K_NO_WAIT);
            if (k_msgq_num_used_get(&hids_queue) == 1) { k_work_submit(&hids_work); }
            break;
        default:
            break;
        }
    }
}


static void bas_notify(void)
{
    uint8_t battery_level = bt_bas_get_battery_level();

    battery_level--;
    if (!battery_level) {
        battery_level = 100U;
    }

    bt_bas_set_battery_level(battery_level);
}

/* ========================= main ========================= */

int main(void)
{
    int err;

    LOG_INF("Dual Mode HID Mouse starting...");

    err = dk_leds_init();
    if (err) {
        LOG_ERR("LEDs init failed: %d", err);
        return err;
    }

    err = dk_buttons_init(button_changed);
    if (err) {
        LOG_ERR("Buttons init failed: %d", err);
        return err;
    }

    err = init_usb_hid();
    if (err) {
        LOG_ERR("USB HID init failed: %d", err);
        return err;
    }

    if (IS_ENABLED(CONFIG_BT_HIDS_SECURITY_ENABLED)) {
        err = bt_conn_auth_cb_register(&conn_auth_callbacks);
        if (err) {
            printk("Failed to register authorization callbacks.\n");
            return 0;
        }
        err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
        if (err) {
            printk("Failed to register authorization info callbacks.\n");
            return 0;
        }
    }

    hid_init();

    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed: %d", err);
        return err;
    }
    LOG_INF("Bluetooth initialized");

    k_work_init(&hids_work, mouse_handler);
    k_work_init(&adv_work, advertising_process);
    if (IS_ENABLED(CONFIG_BT_HIDS_SECURITY_ENABLED)) {
        k_work_init(&pairing_work, pairing_process);
    }

    if (IS_ENABLED(CONFIG_SETTINGS)) {
        settings_load();
    }

    ble_ready = true;

    /* Initialize debounced mode work */
    k_work_init_delayable(&mode_work, mode_worker);
    desired_mode = DESIRED_NONE;

    err = usb_enable(usb_status_callback);
    if (err != 0) {
        LOG_ERR("Failed to enable USB: %d", err);
        return err;
    }

    /* If USB not configured shortly, go to BLE */
    k_sleep(K_MSEC(2000));
    if (!usb_configured) {
        desired_mode = DESIRED_BLE;
        k_work_reschedule(&mode_work, K_NO_WAIT);
    }

    LOG_INF("Dual Mode HID Mouse ready");

    while (true) {
        switch (current_mode) {
        case HID_MODE_USB:
            dk_set_led(DK_LED1, 1);
            dk_set_led(DK_LED2, 0);
            break;
        case HID_MODE_BLE:
            dk_set_led(DK_LED1, 0);
            dk_set_led(DK_LED2, 1);
            break;
        default:
            dk_set_led(DK_LED1, 0);
            dk_set_led(DK_LED2, 0);
            break;
        }

        k_sleep(K_MSEC(100));

        /* Optional: disable during stabilization testing */
        /* bas_notify(); */
    }

    return 0;
}
