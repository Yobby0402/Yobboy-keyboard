#include "keyboard_transport.h"

#include <string.h>
#include "keyboard_ble_hid.h"
#include "keyboard_power.h"
#include "keyboard_power_policy.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "usb_descriptors.h"

static const char *TAG = "keyboard_transport";

static keyboard_transport_status_t s_status = {
    .preferred_mode = KEYBOARD_TRANSPORT_MODE_USB,
    .active_mode = KEYBOARD_TRANSPORT_MODE_USB,
};

static void refresh_active_mode(void)
{
    if (s_status.preferred_mode == KEYBOARD_TRANSPORT_MODE_USB) {
        s_status.active_mode = s_status.usb_mounted
            ? KEYBOARD_TRANSPORT_MODE_USB
            : (s_status.ble_connected ? KEYBOARD_TRANSPORT_MODE_BLE : KEYBOARD_TRANSPORT_MODE_USB);
        return;
    }

    s_status.active_mode = s_status.ble_connected
        ? KEYBOARD_TRANSPORT_MODE_BLE
        : (s_status.usb_mounted ? KEYBOARD_TRANSPORT_MODE_USB : KEYBOARD_TRANSPORT_MODE_BLE);
}

esp_err_t keyboard_transport_init(void)
{
    memset(&s_status, 0, sizeof(s_status));
    s_status.preferred_mode = KEYBOARD_TRANSPORT_MODE_USB;
    s_status.active_mode = KEYBOARD_TRANSPORT_MODE_USB;
    esp_err_t ret = keyboard_ble_hid_init();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "BLE HID transport initialized");
    } else if (ret != ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGW(TAG, "BLE HID transport init failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "BLE HID transport disabled in current sdkconfig");
    }
    return ESP_OK;
}

void keyboard_transport_set_preferred_mode(keyboard_transport_mode_t mode)
{
    s_status.preferred_mode = mode;
    refresh_active_mode();
    ESP_LOGI(TAG, "Preferred transport mode: %s",
             mode == KEYBOARD_TRANSPORT_MODE_BLE ? "BLE" : "USB");
}

keyboard_transport_mode_t keyboard_transport_get_active_mode(void)
{
    return s_status.active_mode;
}

const keyboard_transport_status_t *keyboard_transport_get_status(void)
{
    return &s_status;
}

void keyboard_transport_notify_usb_mount(void)
{
    s_status.usb_mounted = true;
    s_status.usb_suspended = false;
    refresh_active_mode();
    keyboard_power_note_activity();
    keyboard_power_policy_note_activity();
}

void keyboard_transport_notify_usb_unmount(void)
{
    s_status.usb_mounted = false;
    s_status.usb_suspended = false;
    s_status.usb_remote_wakeup_enabled = false;
    refresh_active_mode();
    keyboard_power_note_activity();
    keyboard_power_policy_note_activity();
}

void keyboard_transport_notify_usb_suspend(bool remote_wakeup_enabled)
{
    s_status.usb_suspended = true;
    s_status.usb_remote_wakeup_enabled = remote_wakeup_enabled;
    keyboard_power_note_activity();
    keyboard_power_policy_note_activity();
}

void keyboard_transport_notify_usb_resume(void)
{
    s_status.usb_suspended = false;
    keyboard_power_note_activity();
    keyboard_power_policy_note_activity();
}

void keyboard_transport_notify_ble_connected(bool connected)
{
    s_status.ble_connected = connected;
    refresh_active_mode();
    keyboard_power_note_activity();
    keyboard_power_policy_note_activity();
    ESP_LOGI(TAG, "BLE transport %s", connected ? "connected" : "disconnected");
}

bool keyboard_transport_request_remote_wakeup(void)
{
    if (s_status.active_mode != KEYBOARD_TRANSPORT_MODE_USB ||
        !s_status.usb_suspended ||
        !s_status.usb_remote_wakeup_enabled) {
        return false;
    }
    return tud_remote_wakeup();
}

bool keyboard_transport_send_keyboard_report(hid_keyboard_modifier_bm_t modifier,
                                            const uint8_t keycodes[6])
{
    if (s_status.active_mode == KEYBOARD_TRANSPORT_MODE_BLE) {
        return keyboard_ble_hid_send_keyboard_report(modifier, keycodes);
    }

    return tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, modifier, keycodes);
}

bool keyboard_transport_send_consumer_report(uint16_t usage_code)
{
    if (s_status.active_mode == KEYBOARD_TRANSPORT_MODE_BLE) {
        return keyboard_ble_hid_send_consumer_report(usage_code);
    }

    uint8_t report[2] = {
        (uint8_t)(usage_code & 0xFF),
        (uint8_t)(usage_code >> 8),
    };
    return tud_hid_report(REPORT_ID_CONSUMER_CONTROL, report, sizeof(report));
}
