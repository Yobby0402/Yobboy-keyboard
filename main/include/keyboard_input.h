#ifndef _KEYBOARD_INPUT_H_
#define _KEYBOARD_INPUT_H_

#include <stdbool.h>
#include <stdint.h>
#include "class/hid/hid_device.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KEYBOARD_INPUT_MAX_KEYS 6
#define KEYBOARD_INPUT_MAX_CONSUMERS 8
#define KEYBOARD_INPUT_MAX_LED_ACTIONS 8
#define KEYBOARD_INPUT_MAX_POWER_ACTIONS 4

typedef struct {
    hid_keyboard_modifier_bm_t modifier;
    uint8_t keycodes[KEYBOARD_INPUT_MAX_KEYS];
    uint8_t keycode_count;
    bool fn_pressed;
    uint16_t consumer_usages[KEYBOARD_INPUT_MAX_CONSUMERS];
    uint8_t consumer_count;
    uint8_t led_actions[KEYBOARD_INPUT_MAX_LED_ACTIONS];
    uint8_t led_action_count;
    uint8_t power_actions[KEYBOARD_INPUT_MAX_POWER_ACTIONS];
    uint8_t power_action_count;
} keyboard_input_report_t;

void keyboard_input_build_report(const int *pressed_pins, int num_pressed_pins,
                                 keyboard_input_report_t *report);

#ifdef __cplusplus
}
#endif

#endif // _KEYBOARD_INPUT_H_
