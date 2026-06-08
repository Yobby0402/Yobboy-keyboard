#ifndef _KEYBOARD_BLE_CONFIG_SERVICE_H_
#define _KEYBOARD_BLE_CONFIG_SERVICE_H_

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define YBK_BLE_CONFIG_SERVICE_UUID    "18f08b8e-a41d-4f5e-b0f2-1111c0de0001"
#define YBK_BLE_CONFIG_STATUS_UUID     "18f08b8e-a41d-4f5e-b0f2-1111c0de0002"
#define YBK_BLE_CONFIG_RX_UUID         "18f08b8e-a41d-4f5e-b0f2-1111c0de0003"
#define YBK_BLE_CONFIG_TX_UUID         "18f08b8e-a41d-4f5e-b0f2-1111c0de0004"

esp_err_t keyboard_ble_config_service_init(void);
void keyboard_ble_config_service_notify_status(void);

#ifdef __cplusplus
}
#endif

#endif // _KEYBOARD_BLE_CONFIG_SERVICE_H_
