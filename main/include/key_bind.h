#include <stdint.h>
#include <stdio.h>

#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "led.h"

#define NUM_MODIFIER_KEYS (sizeof(modifier_key_mappings) / sizeof(modifier_mapping_t))
#define MAX_PRESSED_KEYS 6

/**
 * @brief Define basic key number
 */
typedef struct{
    uint8_t config_key_num;     // Key number defined in Kconfig
    uint8_t tusb_hid_key_num;   // Key code defined in hid.h
}basic_key_mapping_t;

/**
 * @brief Define modifier keys' numbers and their codes, such as ctrl alt and shift.
 */
typedef struct {
    uint8_t key_num;
    hid_keyboard_modifier_bm_t hid_modifier;
} modifier_mapping_t;


/**
 * @brief Double layer key
 */
typedef struct {
    uint8_t key_number;          // Key's number
    uint8_t secondary_key_value; // Another keycode(when pressed with Fn)
} double_layer_key_t;

void process_key_press(int* pressed_pins, int num_pressed_pins);
