#include "keyboard_ble_config_service.h"

#include <string.h>
#include "sdkconfig.h"

#if CONFIG_BT_ENABLED && CONFIG_BT_BLUEDROID_ENABLED && CONFIG_BT_BLE_ENABLED && (CONFIG_GATTS_ENABLE || CONFIG_BT_GATTS_ENABLE)

#include "config_protocol.h"
#include "esp_check.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
#include "keyboard_power_policy.h"
#include "keyboard_profile.h"
#include "keyboard_transport.h"

static const char *TAG = "keyboard_ble_cfg";

#define YBK_BLE_CFG_APP_ID 0x42
#define YBK_BLE_CFG_MTU_CHUNK 180

enum {
    IDX_SVC,
    IDX_STATUS_CHAR,
    IDX_STATUS_VAL,
    IDX_STATUS_CCC,
    IDX_RX_CHAR,
    IDX_RX_VAL,
    IDX_TX_CHAR,
    IDX_TX_VAL,
    IDX_TX_CCC,
    IDX_TOTAL,
};

enum {
    YBK_BLE_STATUS_FLAG_USB_MOUNTED = 1 << 0,
    YBK_BLE_STATUS_FLAG_USB_SUSPENDED = 1 << 1,
    YBK_BLE_STATUS_FLAG_USB_REMOTE_WAKE = 1 << 2,
    YBK_BLE_STATUS_FLAG_BLE_CONNECTED = 1 << 3,
    YBK_BLE_STATUS_FLAG_BLE_ADVERTISING = 1 << 4,
    YBK_BLE_STATUS_FLAG_BLE_SUSPENDED = 1 << 5,
    YBK_BLE_STATUS_FLAG_IDLE_LOW_SCAN = 1 << 6,
    YBK_BLE_STATUS_FLAG_LIGHTING_PAUSED = 1 << 7,
};

typedef struct {
    uint8_t protocol_version;
    uint8_t active_transport;
    uint8_t current_mode;
    uint8_t active_scan_interval_ms;
    uint8_t flags;
    uint8_t reserved[3];
    uint32_t profile_checksum;
    uint32_t idle_ms;
} __attribute__((packed)) ybk_ble_config_status_t;

static uint16_t s_handles[IDX_TOTAL] = {0};
static esp_gatt_if_t s_gatts_if = ESP_GATT_IF_NONE;
static uint16_t s_conn_id = 0;
static bool s_connected = false;
static bool s_service_ready = false;
static bool s_status_notify_enabled = false;
static bool s_tx_notify_enabled = false;
static bool s_registered = false;
static esp_bd_addr_t s_remote_bda = {0};

static uint8_t s_service_uuid[16] = {
    0x01, 0x00, 0xde, 0xc0, 0x11, 0x11, 0xf2, 0xb0,
    0x5e, 0x4f, 0x1d, 0xa4, 0x8e, 0x8b, 0xf0, 0x18,
};
static uint8_t s_status_uuid[16] = {
    0x02, 0x00, 0xde, 0xc0, 0x11, 0x11, 0xf2, 0xb0,
    0x5e, 0x4f, 0x1d, 0xa4, 0x8e, 0x8b, 0xf0, 0x18,
};
static uint8_t s_rx_uuid[16] = {
    0x03, 0x00, 0xde, 0xc0, 0x11, 0x11, 0xf2, 0xb0,
    0x5e, 0x4f, 0x1d, 0xa4, 0x8e, 0x8b, 0xf0, 0x18,
};
static uint8_t s_tx_uuid[16] = {
    0x04, 0x00, 0xde, 0xc0, 0x11, 0x11, 0xf2, 0xb0,
    0x5e, 0x4f, 0x1d, 0xa4, 0x8e, 0x8b, 0xf0, 0x18,
};

static uint16_t s_primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static uint16_t s_char_decl_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static uint16_t s_ccc_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static uint8_t s_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static uint8_t s_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
static uint8_t s_prop_tx = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static uint16_t s_ccc_default = 0x0000;

static uint8_t s_status_value[sizeof(ybk_ble_config_status_t)] = {0};
static uint8_t s_rx_value[YBK_BLE_CFG_MTU_CHUNK] = {0};
static uint8_t s_tx_value[YBK_BLE_CFG_MTU_CHUNK] = {0};

static const esp_gatts_attr_db_t s_attr_db[IDX_TOTAL] = {
    [IDX_SVC] = {
        { ESP_GATT_AUTO_RSP },
        {
            ESP_UUID_LEN_16,
            (uint8_t *)&s_primary_service_uuid,
            ESP_GATT_PERM_READ,
            sizeof(s_service_uuid),
            sizeof(s_service_uuid),
            s_service_uuid,
        },
    },
    [IDX_STATUS_CHAR] = {
        { ESP_GATT_AUTO_RSP },
        {
            ESP_UUID_LEN_16,
            (uint8_t *)&s_char_decl_uuid,
            ESP_GATT_PERM_READ,
            sizeof(uint8_t),
            sizeof(uint8_t),
            &s_prop_read_notify,
        },
    },
    [IDX_STATUS_VAL] = {
        { ESP_GATT_RSP_BY_APP },
        {
            ESP_UUID_LEN_128,
            s_status_uuid,
            ESP_GATT_PERM_READ,
            sizeof(s_status_value),
            sizeof(s_status_value),
            s_status_value,
        },
    },
    [IDX_STATUS_CCC] = {
        { ESP_GATT_AUTO_RSP },
        {
            ESP_UUID_LEN_16,
            (uint8_t *)&s_ccc_uuid,
            ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
            sizeof(s_ccc_default),
            sizeof(s_ccc_default),
            (uint8_t *)&s_ccc_default,
        },
    },
    [IDX_RX_CHAR] = {
        { ESP_GATT_AUTO_RSP },
        {
            ESP_UUID_LEN_16,
            (uint8_t *)&s_char_decl_uuid,
            ESP_GATT_PERM_READ,
            sizeof(uint8_t),
            sizeof(uint8_t),
            &s_prop_write,
        },
    },
    [IDX_RX_VAL] = {
        { ESP_GATT_AUTO_RSP },
        {
            ESP_UUID_LEN_128,
            s_rx_uuid,
            ESP_GATT_PERM_WRITE,
            sizeof(s_rx_value),
            0,
            s_rx_value,
        },
    },
    [IDX_TX_CHAR] = {
        { ESP_GATT_AUTO_RSP },
        {
            ESP_UUID_LEN_16,
            (uint8_t *)&s_char_decl_uuid,
            ESP_GATT_PERM_READ,
            sizeof(uint8_t),
            sizeof(uint8_t),
            &s_prop_tx,
        },
    },
    [IDX_TX_VAL] = {
        { ESP_GATT_AUTO_RSP },
        {
            ESP_UUID_LEN_128,
            s_tx_uuid,
            ESP_GATT_PERM_READ,
            sizeof(s_tx_value),
            0,
            s_tx_value,
        },
    },
    [IDX_TX_CCC] = {
        { ESP_GATT_AUTO_RSP },
        {
            ESP_UUID_LEN_16,
            (uint8_t *)&s_ccc_uuid,
            ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
            sizeof(s_ccc_default),
            sizeof(s_ccc_default),
            (uint8_t *)&s_ccc_default,
        },
    },
};

static void build_status(ybk_ble_config_status_t *status)
{
    const keyboard_transport_status_t *transport = keyboard_transport_get_status();
    const keyboard_power_policy_status_t *policy = keyboard_power_policy_get_status();

    memset(status, 0, sizeof(*status));
    status->protocol_version = YBK_CONFIG_PROTOCOL_VERSION;
    status->active_transport = (uint8_t)transport->active_mode;
    status->current_mode = (uint8_t)policy->current_mode;
    status->active_scan_interval_ms = policy->active_scan_interval_ms;
    status->reserved[0] = keyboard_profile_socd_enabled() ? 1 : 0;
    status->reserved[1] = keyboard_profile_reverse_tap_enabled() ? 1 : 0;
    status->profile_checksum = keyboard_profile_get_checksum();
    status->idle_ms = policy->idle_ms;

    if (transport->usb_mounted) {
        status->flags |= YBK_BLE_STATUS_FLAG_USB_MOUNTED;
    }
    if (transport->usb_suspended) {
        status->flags |= YBK_BLE_STATUS_FLAG_USB_SUSPENDED;
    }
    if (transport->usb_remote_wakeup_enabled) {
        status->flags |= YBK_BLE_STATUS_FLAG_USB_REMOTE_WAKE;
    }
    if (transport->ble_connected) {
        status->flags |= YBK_BLE_STATUS_FLAG_BLE_CONNECTED;
    }
    if (transport->ble_advertising) {
        status->flags |= YBK_BLE_STATUS_FLAG_BLE_ADVERTISING;
    }
    if (transport->ble_suspended) {
        status->flags |= YBK_BLE_STATUS_FLAG_BLE_SUSPENDED;
    }
    if (policy->idle_low_scan_active) {
        status->flags |= YBK_BLE_STATUS_FLAG_IDLE_LOW_SCAN;
    }
    if (policy->lighting_paused) {
        status->flags |= YBK_BLE_STATUS_FLAG_LIGHTING_PAUSED;
    }
}

static esp_err_t send_status_notification(void)
{
    if (!s_connected || !s_service_ready || !s_status_notify_enabled || s_gatts_if == ESP_GATT_IF_NONE) {
        return ESP_ERR_INVALID_STATE;
    }

    ybk_ble_config_status_t status = {0};
    build_status(&status);
    memcpy(s_status_value, &status, sizeof(status));

    return esp_ble_gatts_send_indicate(s_gatts_if, s_conn_id, s_handles[IDX_STATUS_VAL],
                                       sizeof(status), s_status_value, false);
}

static esp_err_t ble_protocol_write(const uint8_t *data, size_t len, void *ctx)
{
    (void)ctx;
    if (!s_connected || !s_service_ready || !s_tx_notify_enabled || s_gatts_if == ESP_GATT_IF_NONE) {
        return ESP_ERR_INVALID_STATE;
    }

    size_t offset = 0;
    while (offset < len) {
        size_t chunk = len - offset;
        if (chunk > YBK_BLE_CFG_MTU_CHUNK) {
            chunk = YBK_BLE_CFG_MTU_CHUNK;
        }

        esp_err_t ret = esp_ble_gatts_send_indicate(
            s_gatts_if, s_conn_id, s_handles[IDX_TX_VAL], chunk, (uint8_t *)(data + offset), false);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "BLE TX notify failed after %u bytes: %s",
                     (unsigned)offset, esp_err_to_name(ret));
            return ret;
        }
        offset += chunk;
    }

    return ESP_OK;
}

static void request_balanced_conn_params(const esp_bd_addr_t remote_bda)
{
    esp_ble_conn_update_params_t params = {
        .min_int = 0x000A, // 12.5 ms
        .max_int = 0x0012, // 22.5 ms
        .latency = 4,
        .timeout = 500,
    };
    memcpy(params.bda, remote_bda, sizeof(params.bda));
    esp_err_t ret = esp_ble_gap_update_conn_params(&params);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "BLE conn param update request failed: %s", esp_err_to_name(ret));
    }
}

static bool ccc_enabled(const uint8_t *value, uint16_t len)
{
    if (value == NULL || len < 2) {
        return false;
    }
    uint16_t ccc = (uint16_t)value[0] | ((uint16_t)value[1] << 8);
    return (ccc & 0x0001u) != 0;
}

static void handle_read_event(esp_ble_gatts_cb_param_t *param)
{
    if (param->read.handle != s_handles[IDX_STATUS_VAL]) {
        esp_ble_gatts_send_response(s_gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, NULL);
        return;
    }

    ybk_ble_config_status_t status = {0};
    esp_gatt_rsp_t rsp = {0};
    build_status(&status);
    rsp.attr_value.handle = param->read.handle;
    rsp.attr_value.len = sizeof(status);
    memcpy(rsp.attr_value.value, &status, sizeof(status));
    esp_ble_gatts_send_response(s_gatts_if, param->read.conn_id, param->read.trans_id,
                                ESP_GATT_OK, &rsp);
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        s_gatts_if = gatts_if;
        esp_ble_gatts_create_attr_tab(s_attr_db, gatts_if, IDX_TOTAL, YBK_BLE_CFG_APP_ID);
        break;
    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
        if (param->add_attr_tab.status != ESP_GATT_OK || param->add_attr_tab.num_handle != IDX_TOTAL) {
            ESP_LOGE(TAG, "BLE config attr table create failed: status=%d handles=%d",
                     param->add_attr_tab.status, param->add_attr_tab.num_handle);
            break;
        }
        memcpy(s_handles, param->add_attr_tab.handles, sizeof(s_handles));
        s_service_ready = true;
        esp_ble_gatts_start_service(s_handles[IDX_SVC]);
        ESP_LOGI(TAG, "BLE config service ready");
        keyboard_ble_config_service_notify_status();
        break;
    case ESP_GATTS_CONNECT_EVT:
        s_connected = true;
        s_conn_id = param->connect.conn_id;
        memcpy(s_remote_bda, param->connect.remote_bda, sizeof(s_remote_bda));
        s_status_notify_enabled = false;
        s_tx_notify_enabled = false;
        request_balanced_conn_params(param->connect.remote_bda);
        keyboard_ble_config_service_notify_status();
        break;
    case ESP_GATTS_DISCONNECT_EVT:
        s_connected = false;
        s_status_notify_enabled = false;
        s_tx_notify_enabled = false;
        memset(s_remote_bda, 0, sizeof(s_remote_bda));
        break;
    case ESP_GATTS_READ_EVT:
        handle_read_event(param);
        break;
    case ESP_GATTS_WRITE_EVT:
        if (param->write.is_prep) {
            break;
        }
        if (param->write.handle == s_handles[IDX_STATUS_CCC]) {
            s_status_notify_enabled = ccc_enabled(param->write.value, param->write.len);
            if (s_status_notify_enabled) {
                keyboard_ble_config_service_notify_status();
            }
            break;
        }
        if (param->write.handle == s_handles[IDX_TX_CCC]) {
            s_tx_notify_enabled = ccc_enabled(param->write.value, param->write.len);
            break;
        }
        if (param->write.handle == s_handles[IDX_RX_VAL] && param->write.len > 0) {
            esp_err_t ret = config_protocol_process_external_bytes(
                param->write.value, param->write.len, ble_protocol_write, NULL);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "BLE config RX parse failed: %s", esp_err_to_name(ret));
            }
        }
        break;
    default:
        break;
    }
}

esp_err_t keyboard_ble_config_service_init(void)
{
    if (s_registered) {
        return ESP_OK;
    }

    esp_err_t ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    ret = esp_ble_gatts_app_register(YBK_BLE_CFG_APP_ID);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    s_registered = true;
    return ESP_OK;
}

void keyboard_ble_config_service_notify_status(void)
{
    if (!s_service_ready) {
        return;
    }
    if (send_status_notification() != ESP_OK) {
        ybk_ble_config_status_t status = {0};
        build_status(&status);
        memcpy(s_status_value, &status, sizeof(status));
    }
}

#else

esp_err_t keyboard_ble_config_service_init(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

void keyboard_ble_config_service_notify_status(void)
{
}

#endif
