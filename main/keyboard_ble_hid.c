#include "keyboard_ble_hid.h"

#include <string.h>
#include "sdkconfig.h"

#if CONFIG_BT_ENABLED && CONFIG_BT_BLUEDROID_ENABLED && CONFIG_BT_BLE_ENABLED && (CONFIG_GATTS_ENABLE || CONFIG_BT_GATTS_ENABLE)

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_check.h"
#include "esp_gap_ble_api.h"
#include "esp_hid_common.h"
#include "esp_hidd.h"
#include "esp_log.h"
#include "keyboard_ble_config_service.h"
#include "keyboard_layout_meta.h"
#include "keyboard_transport.h"
#include "usb_descriptors.h"

static const char *TAG = "keyboard_ble_hid";

#define BLE_HID_VENDOR_ID     0x303A
#define BLE_HID_PRODUCT_ID    0x4002
#define BLE_HID_VERSION       0x0100
static char s_ble_device_name[YBK_BLE_DEVICE_NAME_LEN] = "Yobboy Keyboard BLE";

static esp_hidd_dev_t *s_hid_dev = NULL;
static bool s_stack_ready = false;
static bool s_adv_data_ready = false;
static bool s_adv_start_pending = false;

static const uint8_t s_keyboard_ble_report_map[] = {
    0x05, 0x01,                         // Usage Page (Generic Desktop)
    0x09, 0x06,                         // Usage (Keyboard)
    0xA1, 0x01,                         // Collection (Application)
    0x85, REPORT_ID_KEYBOARD,           //   Report ID (Keyboard)
    0x05, 0x07,                         //   Usage Page (Keyboard/Keypad)
    0x19, 0xE0,                         //   Usage Minimum (224)
    0x29, 0xE7,                         //   Usage Maximum (231)
    0x15, 0x00,                         //   Logical Minimum (0)
    0x25, 0x01,                         //   Logical Maximum (1)
    0x75, 0x01,                         //   Report Size (1)
    0x95, 0x08,                         //   Report Count (8)
    0x81, 0x02,                         //   Input (Data,Var,Abs)
    0x95, 0x01,                         //   Report Count (1)
    0x75, 0x08,                         //   Report Size (8)
    0x81, 0x01,                         //   Input (Const,Array,Abs)
    0x95, 0x05,                         //   Report Count (5)
    0x75, 0x01,                         //   Report Size (1)
    0x05, 0x08,                         //   Usage Page (LEDs)
    0x19, 0x01,                         //   Usage Minimum (Num Lock)
    0x29, 0x05,                         //   Usage Maximum (Kana)
    0x91, 0x02,                         //   Output (Data,Var,Abs)
    0x95, 0x01,                         //   Report Count (1)
    0x75, 0x03,                         //   Report Size (3)
    0x91, 0x01,                         //   Output (Const,Array,Abs)
    0x95, 0x06,                         //   Report Count (6)
    0x75, 0x08,                         //   Report Size (8)
    0x15, 0x00,                         //   Logical Minimum (0)
    0x26, 0xA4, 0x00,                   //   Logical Maximum (164)
    0x05, 0x07,                         //   Usage Page (Keyboard/Keypad)
    0x19, 0x00,                         //   Usage Minimum (0)
    0x29, 0xA4,                         //   Usage Maximum (164)
    0x81, 0x00,                         //   Input (Data,Array,Abs)
    0xC0,                               // End Collection

    0x05, 0x0C,                         // Usage Page (Consumer)
    0x09, 0x01,                         // Usage (Consumer Control)
    0xA1, 0x01,                         // Collection (Application)
    0x85, REPORT_ID_CONSUMER_CONTROL,   //   Report ID (Consumer)
    0x15, 0x00,                         //   Logical Minimum (0)
    0x26, 0xFF, 0x03,                   //   Logical Maximum (1023)
    0x19, 0x00,                         //   Usage Minimum (0)
    0x2A, 0xFF, 0x03,                   //   Usage Maximum (1023)
    0x75, 0x10,                         //   Report Size (16)
    0x95, 0x01,                         //   Report Count (1)
    0x81, 0x00,                         //   Input (Data,Array,Abs)
    0xC0                                // End Collection
};

static esp_hid_raw_report_map_t s_report_maps[] = {
    {
        .data = s_keyboard_ble_report_map,
        .len = sizeof(s_keyboard_ble_report_map),
    },
};

static esp_hid_device_config_t s_ble_hid_config = {
    .vendor_id = BLE_HID_VENDOR_ID,
    .product_id = BLE_HID_PRODUCT_ID,
    .version = BLE_HID_VERSION,
    .device_name = s_ble_device_name,
    .manufacturer_name = CONFIG_TINYUSB_DESC_MANUFACTURER_STRING,
    .serial_number = CONFIG_TINYUSB_DESC_SERIAL_STRING,
    .report_maps = s_report_maps,
    .report_maps_len = 1,
};

static void init_ble_device_name(void)
{
    const keyboard_layout_meta_t *meta = keyboard_layout_meta_get();
    strncpy(s_ble_device_name, meta->ble_device_name, sizeof(s_ble_device_name) - 1);
    s_ble_device_name[sizeof(s_ble_device_name) - 1] = '\0';
}

static esp_err_t ble_start_advertising(void)
{
    static esp_ble_adv_params_t adv_params = {
        .adv_int_min = 0x20,
        .adv_int_max = 0x30,
        .adv_type = ADV_TYPE_IND,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .channel_map = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };

    if (!s_adv_data_ready) {
        s_adv_start_pending = true;
        return ESP_OK;
    }

    s_adv_start_pending = false;
    return esp_ble_gap_start_advertising(&adv_params);
}

static void ble_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        s_adv_data_ready = true;
        ESP_LOGI(TAG, "BLE advertising payload ready");
        if (s_adv_start_pending) {
            esp_err_t ret = ble_start_advertising();
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "BLE advertising start failed after adv setup: %s", esp_err_to_name(ret));
            }
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            keyboard_transport_notify_ble_advertising(true);
            ESP_LOGI(TAG, "BLE advertising started");
        } else {
            keyboard_transport_notify_ble_advertising(false);
            ESP_LOGW(TAG, "BLE advertising start error: %d", param->adv_start_cmpl.status);
        }
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        ESP_LOGI(TAG, "BLE security request");
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        if (param->ble_security.auth_cmpl.success) {
            ESP_LOGI(TAG, "BLE authentication complete");
        } else {
            ESP_LOGW(TAG, "BLE authentication failed: 0x%x", param->ble_security.auth_cmpl.fail_reason);
        }
        break;
    default:
        break;
    }
}

static void ble_hidd_event_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    (void)handler_args;
    (void)base;

    esp_hidd_event_t event = (esp_hidd_event_t)id;
    esp_hidd_event_data_t *param = (esp_hidd_event_data_t *)event_data;

    switch (event) {
    case ESP_HIDD_START_EVENT:
        ESP_LOGI(TAG, "BLE HID started");
        if (ble_start_advertising() != ESP_OK) {
            ESP_LOGW(TAG, "BLE advertising request failed");
        }
        break;
    case ESP_HIDD_CONNECT_EVENT:
        ESP_LOGI(TAG, "BLE HID connected");
        keyboard_transport_notify_ble_advertising(false);
        keyboard_transport_notify_ble_suspend(false);
        keyboard_transport_notify_ble_connected(true);
        if (param->connect.dev) {
            esp_hidd_dev_battery_set(param->connect.dev, 100);
        }
        break;
    case ESP_HIDD_DISCONNECT_EVENT:
        ESP_LOGI(TAG, "BLE HID disconnected: %s",
                 esp_hid_disconnect_reason_str(ESP_HID_TRANSPORT_BLE, param->disconnect.reason));
        keyboard_transport_notify_ble_suspend(false);
        keyboard_transport_notify_ble_connected(false);
        if (ble_start_advertising() != ESP_OK) {
            ESP_LOGW(TAG, "BLE advertising restart failed");
        }
        break;
    case ESP_HIDD_CONTROL_EVENT:
        keyboard_transport_notify_ble_suspend(
            param->control.control != ESP_HID_CONTROL_EXIT_SUSPEND);
        ESP_LOGI(TAG, "BLE HID control: %s",
                 param->control.control == ESP_HID_CONTROL_EXIT_SUSPEND ? "exit suspend" : "suspend");
        break;
    case ESP_HIDD_STOP_EVENT:
        ESP_LOGI(TAG, "BLE HID stopped");
        keyboard_transport_notify_ble_advertising(false);
        keyboard_transport_notify_ble_suspend(false);
        keyboard_transport_notify_ble_connected(false);
        break;
    default:
        break;
    }
}

static esp_err_t ble_stack_init(void)
{
    esp_err_t ret;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();

    ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_bt_controller_mem_release failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_bt_controller_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_bt_controller_enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_init_with_cfg(&bluedroid_cfg);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_bluedroid_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_bluedroid_enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_ble_gap_register_callback(ble_gap_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_register_callback failed: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

static esp_err_t ble_gap_configure(void)
{
    static const uint8_t hid_service_uuid128[] = {
        0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
        0x00, 0x10, 0x00, 0x00, 0x12, 0x18, 0x00, 0x00,
    };

    esp_ble_adv_data_t adv_data = {
        .set_scan_rsp = false,
        .include_name = true,
        .include_txpower = true,
        .appearance = ESP_HID_APPEARANCE_KEYBOARD,
        .service_uuid_len = sizeof(hid_service_uuid128),
        .p_service_uuid = (uint8_t *)hid_service_uuid128,
        .flag = ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT,
    };

    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t key_size = 16;

    ESP_RETURN_ON_ERROR(
        esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(auth_req)),
        TAG,
        "set auth mode failed");
    ESP_RETURN_ON_ERROR(
        esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(iocap)),
        TAG,
        "set io capability failed");
    ESP_RETURN_ON_ERROR(
        esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(init_key)),
        TAG,
        "set init key failed");
    ESP_RETURN_ON_ERROR(
        esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(rsp_key)),
        TAG,
        "set rsp key failed");
    ESP_RETURN_ON_ERROR(
        esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(key_size)),
        TAG,
        "set key size failed");
    ESP_RETURN_ON_ERROR(esp_ble_gap_set_device_name(s_ble_device_name), TAG, "set device name failed");

    s_adv_data_ready = false;
    s_adv_start_pending = false;
    return esp_ble_gap_config_adv_data(&adv_data);
}

esp_err_t keyboard_ble_hid_init(void)
{
    if (s_hid_dev != NULL) {
        return ESP_OK;
    }

    if (!s_stack_ready) {
        ESP_RETURN_ON_ERROR(ble_stack_init(), TAG, "BLE stack init failed");
        s_stack_ready = true;
    }

    init_ble_device_name();
    ESP_RETURN_ON_ERROR(ble_gap_configure(), TAG, "BLE GAP configure failed");
    ESP_RETURN_ON_ERROR(keyboard_ble_config_service_init(), TAG, "BLE config service init failed");
    ESP_RETURN_ON_ERROR(
        esp_hidd_dev_init(&s_ble_hid_config, ESP_HID_TRANSPORT_BLE, ble_hidd_event_callback, &s_hid_dev),
        TAG,
        "BLE HID init failed");

    ESP_LOGI(TAG, "BLE HID initialized");
    return ESP_OK;
}

bool keyboard_ble_hid_connected(void)
{
    return s_hid_dev != NULL && esp_hidd_dev_connected(s_hid_dev);
}

bool keyboard_ble_hid_send_keyboard_report(hid_keyboard_modifier_bm_t modifier,
                                           const uint8_t keycodes[6])
{
    if (!keyboard_ble_hid_connected()) {
        return false;
    }

    uint8_t report[8] = {0};
    report[0] = (uint8_t)modifier;
    if (keycodes != NULL) {
        memcpy(&report[2], keycodes, 6);
    }

    return esp_hidd_dev_input_set(s_hid_dev, 0, REPORT_ID_KEYBOARD, report, sizeof(report)) == ESP_OK;
}

bool keyboard_ble_hid_send_consumer_report(uint16_t usage_code)
{
    if (!keyboard_ble_hid_connected()) {
        return false;
    }

    uint8_t report[2] = {
        (uint8_t)(usage_code & 0xFF),
        (uint8_t)(usage_code >> 8),
    };

    return esp_hidd_dev_input_set(s_hid_dev, 0, REPORT_ID_CONSUMER_CONTROL, report, sizeof(report)) == ESP_OK;
}

#else

esp_err_t keyboard_ble_hid_init(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

bool keyboard_ble_hid_connected(void)
{
    return false;
}

bool keyboard_ble_hid_send_keyboard_report(hid_keyboard_modifier_bm_t modifier,
                                           const uint8_t keycodes[6])
{
    (void)modifier;
    (void)keycodes;
    return false;
}

bool keyboard_ble_hid_send_consumer_report(uint16_t usage_code)
{
    (void)usage_code;
    return false;
}

#endif
