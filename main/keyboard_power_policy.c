#include "keyboard_power_policy.h"

#include "esp_log.h"
#include "keyboard_ble_config_service.h"
#include "keyboard_power.h"

static const char *TAG = "keyboard_power_policy";

static keyboard_power_policy_status_t s_status = {
    .current_mode = YBK_POWER_MODE_OFFICE,
    .idle_low_scan_active = false,
    .lighting_paused = false,
    .idle_ms = 0,
    .active_scan_interval_ms = 0,
};
static uint32_t s_last_status_idle_sec = 0;

static void notify_ble_status_if_needed(const keyboard_power_policy_status_t *before)
{
    uint32_t idle_sec = s_status.idle_ms / 1000;
    bool changed = before == NULL ||
        before->current_mode != s_status.current_mode ||
        before->idle_low_scan_active != s_status.idle_low_scan_active ||
        before->lighting_paused != s_status.lighting_paused ||
        before->active_scan_interval_ms != s_status.active_scan_interval_ms ||
        s_last_status_idle_sec != idle_sec;
    if (!changed) {
        return;
    }

    s_last_status_idle_sec = idle_sec;
    keyboard_ble_config_service_notify_status();
}

static const char *mode_name(keyboard_power_mode_t mode)
{
    switch (mode) {
    case YBK_POWER_MODE_GAME:
        return "game";
    case YBK_POWER_MODE_SAVER:
        return "saver";
    case YBK_POWER_MODE_OFFICE:
    default:
        return "office";
    }
}

static bool transport_allows_dynamic_power(const keyboard_transport_status_t *transport)
{
    if (transport == NULL || transport->usb_mounted) {
        return false;
    }

    return transport->active_mode == KEYBOARD_TRANSPORT_MODE_BLE ||
           transport->preferred_mode == KEYBOARD_TRANSPORT_MODE_BLE ||
           transport->ble_connected;
}

static bool transport_forces_low_scan(const keyboard_transport_status_t *transport)
{
    return transport != NULL && transport->ble_suspended;
}

static void refresh_scan_state(const keyboard_transport_status_t *transport)
{
    s_status.current_mode = keyboard_profile_get_power_mode();
    bool low_scan = s_status.idle_low_scan_active || transport_forces_low_scan(transport);
    s_status.active_scan_interval_ms = low_scan
        ? keyboard_profile_get_idle_scan_interval_ms()
        : keyboard_profile_get_scan_interval_ms();
    s_status.lighting_paused = low_scan;
}

esp_err_t keyboard_power_policy_init(void)
{
    s_status.current_mode = keyboard_profile_get_power_mode();
    s_status.idle_low_scan_active = false;
    s_status.lighting_paused = false;
    s_status.idle_ms = 0;
    refresh_scan_state(NULL);
    s_last_status_idle_sec = 0;

    ESP_LOGI(TAG, "Runtime mode=%s, scan=%u ms, idle scan=%u ms",
             mode_name(s_status.current_mode),
             keyboard_profile_get_scan_interval_ms(),
             keyboard_profile_get_idle_scan_interval_ms());
    return ESP_OK;
}

void keyboard_power_policy_note_activity(void)
{
    keyboard_power_policy_status_t before = s_status;

    if (s_status.idle_low_scan_active) {
        ESP_LOGI(TAG, "Exit idle low scan, restore %s mode (%u ms)",
                 mode_name(keyboard_profile_get_power_mode()),
                 keyboard_profile_get_scan_interval_ms());
    }

    s_status.idle_low_scan_active = false;
    s_status.idle_ms = 0;
    refresh_scan_state(NULL);
    notify_ble_status_if_needed(&before);
}

void keyboard_power_policy_tick(const keyboard_transport_status_t *transport)
{
    keyboard_power_policy_status_t before = s_status;
    s_status.current_mode = keyboard_profile_get_power_mode();
    s_status.idle_ms = keyboard_power_get_idle_ms();

    if (!transport_allows_dynamic_power(transport)) {
        if (s_status.idle_low_scan_active) {
            ESP_LOGI(TAG, "Exit idle low scan due to transport state");
        }
        s_status.idle_low_scan_active = false;
        refresh_scan_state(transport);
        notify_ble_status_if_needed(&before);
        return;
    }

    uint16_t idle_threshold = keyboard_profile_get_idle_enter_low_power_ms();
    bool should_low_scan = idle_threshold > 0 && s_status.idle_ms >= idle_threshold;

    if (should_low_scan && !s_status.idle_low_scan_active) {
        ESP_LOGI(TAG, "Enter idle low scan after %lu ms, scan=%u ms",
                 (unsigned long)s_status.idle_ms,
                 keyboard_profile_get_idle_scan_interval_ms());
    }

    s_status.idle_low_scan_active = should_low_scan;
    refresh_scan_state(transport);
    notify_ble_status_if_needed(&before);
}

bool keyboard_power_policy_cycle_mode(void)
{
    if (!keyboard_profile_cycle_power_mode()) {
        return false;
    }

    s_status.idle_low_scan_active = false;
    refresh_scan_state(NULL);
    ESP_LOGI(TAG, "Runtime mode switched to %s (%u ms)",
             mode_name(s_status.current_mode),
             keyboard_profile_get_scan_interval_ms());
    notify_ble_status_if_needed(NULL);
    return true;
}

uint8_t keyboard_power_policy_get_scan_interval_ms(uint8_t fallback_ms)
{
    if (s_status.active_scan_interval_ms == 0) {
        return fallback_ms;
    }
    return s_status.active_scan_interval_ms;
}

bool keyboard_power_policy_should_update_lighting(void)
{
    return !s_status.lighting_paused;
}

bool keyboard_power_policy_should_enter_deep_sleep(const keyboard_transport_status_t *transport)
{
    uint32_t threshold = keyboard_profile_get_idle_enter_deep_sleep_ms();
    if (!transport_allows_dynamic_power(transport) || threshold == 0) {
        return false;
    }

    return keyboard_power_get_idle_ms() >= threshold;
}

const keyboard_power_policy_status_t *keyboard_power_policy_get_status(void)
{
    return &s_status;
}
