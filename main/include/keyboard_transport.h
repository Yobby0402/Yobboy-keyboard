#ifndef _KEYBOARD_TRANSPORT_H_
#define _KEYBOARD_TRANSPORT_H_

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "class/hid/hid_device.h"
#include "keyboard_input.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    KEYBOARD_TRANSPORT_MODE_USB = 0,
    KEYBOARD_TRANSPORT_MODE_BLE = 1,
} keyboard_transport_mode_t;

typedef struct {
    keyboard_transport_mode_t preferred_mode;
    keyboard_transport_mode_t active_mode;
    bool usb_mounted;
    bool usb_suspended;
    bool usb_remote_wakeup_enabled;
    bool ble_connected;
    bool ble_advertising;
    bool ble_suspended;
} keyboard_transport_status_t;

esp_err_t keyboard_transport_init(void);
void keyboard_transport_set_preferred_mode(keyboard_transport_mode_t mode);
keyboard_transport_mode_t keyboard_transport_get_active_mode(void);
const keyboard_transport_status_t *keyboard_transport_get_status(void);

void keyboard_transport_notify_usb_mount(void);
void keyboard_transport_notify_usb_unmount(void);
void keyboard_transport_notify_usb_suspend(bool remote_wakeup_enabled);
void keyboard_transport_notify_usb_resume(void);
void keyboard_transport_notify_ble_connected(bool connected);
void keyboard_transport_notify_ble_advertising(bool advertising);
void keyboard_transport_notify_ble_suspend(bool suspended);

bool keyboard_transport_request_remote_wakeup(void);
bool keyboard_transport_send_keyboard_report(const keyboard_input_report_t *report);
bool keyboard_transport_send_consumer_report(uint16_t usage_code);

#ifdef __cplusplus
}
#endif

#endif // _KEYBOARD_TRANSPORT_H_
