#ifndef _KEYBOARD_POWER_H_
#define _KEYBOARD_POWER_H_

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_sleep.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool wake_gpio_enabled;
    gpio_num_t wake_gpio;
    bool wake_active_low;
    bool wake_gpio_valid;
    bool deep_sleep_gpio_supported;
    uint32_t idle_light_sleep_ms;
    uint32_t idle_deep_sleep_ms;
    uint64_t last_activity_us;
    esp_sleep_wakeup_cause_t last_wakeup_cause;
} keyboard_power_status_t;

esp_err_t keyboard_power_init(void);
void keyboard_power_note_activity(void);
bool keyboard_power_wake_key_pressed(void);
uint32_t keyboard_power_get_idle_ms(void);
bool keyboard_power_should_enter_light_sleep(bool usb_mounted);
bool keyboard_power_should_enter_deep_sleep(bool usb_mounted);
esp_err_t keyboard_power_prepare_sleep(esp_sleep_mode_t mode);
esp_err_t keyboard_power_enter_light_sleep(void);
void keyboard_power_enter_deep_sleep(void);
const keyboard_power_status_t *keyboard_power_get_status(void);

#ifdef __cplusplus
}
#endif

#endif // _KEYBOARD_POWER_H_
