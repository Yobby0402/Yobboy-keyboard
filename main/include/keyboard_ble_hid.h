#ifndef _KEYBOARD_BLE_HID_H_
#define _KEYBOARD_BLE_HID_H_

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "class/hid/hid_device.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t keyboard_ble_hid_init(void);
bool keyboard_ble_hid_connected(void);
bool keyboard_ble_hid_send_keyboard_report(hid_keyboard_modifier_bm_t modifier,
                                           const uint8_t keycodes[6]);
bool keyboard_ble_hid_send_consumer_report(uint16_t usage_code);

#ifdef __cplusplus
}
#endif

#endif // _KEYBOARD_BLE_HID_H_
