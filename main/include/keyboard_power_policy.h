#ifndef _KEYBOARD_POWER_POLICY_H_
#define _KEYBOARD_POWER_POLICY_H_

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "keyboard_profile.h"
#include "keyboard_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    keyboard_power_mode_t current_mode;
    bool idle_low_scan_active;
    bool lighting_paused;
    uint32_t idle_ms;
    uint8_t active_scan_interval_ms;
} keyboard_power_policy_status_t;

esp_err_t keyboard_power_policy_init(void);
void keyboard_power_policy_note_activity(void);
void keyboard_power_policy_tick(const keyboard_transport_status_t *transport);
bool keyboard_power_policy_cycle_mode(void);
uint8_t keyboard_power_policy_get_scan_interval_ms(uint8_t fallback_ms);
bool keyboard_power_policy_should_update_lighting(void);
bool keyboard_power_policy_should_enter_deep_sleep(const keyboard_transport_status_t *transport);
const keyboard_power_policy_status_t *keyboard_power_policy_get_status(void);

#ifdef __cplusplus
}
#endif

#endif // _KEYBOARD_POWER_POLICY_H_
