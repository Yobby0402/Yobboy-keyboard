#ifndef _KEYBOARD_PROFILE_H_
#define _KEYBOARD_PROFILE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define YBK_PROFILE_MAGIC 0x504B4259u /* "YBKP" little-endian */
#define YBK_PROFILE_VERSION 2
#define YBK_MAX_KEYS 80
#define YBK_LAYER_COUNT 2

typedef enum {
    YBK_LAYER_BASE = 0,
    YBK_LAYER_FN = 1,
} keyboard_layer_t;

typedef enum {
    YBK_ACTION_NONE = 0,
    YBK_ACTION_KEY = 1,
    YBK_ACTION_MODIFIER = 2,
    YBK_ACTION_CONSUMER = 3,
    YBK_ACTION_LAYER_FN = 4,
    YBK_ACTION_LED_TOGGLE = 5,
    YBK_ACTION_LED_BRIGHTNESS_UP = 6,
    YBK_ACTION_LED_BRIGHTNESS_DOWN = 7,
    YBK_ACTION_LED_EFFECT_NEXT = 8,
} keyboard_action_type_t;

typedef enum {
    YBK_LED_MODE_HID = 0,
    YBK_LED_MODE_SOLID = 1,
    YBK_LED_MODE_BLINK = 2,
} keyboard_led_mode_t;

typedef struct {
    uint8_t type;
    uint8_t flags;
    uint16_t code;
} __attribute__((packed)) keyboard_action_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint32_t checksum;

    keyboard_action_t keymap[YBK_LAYER_COUNT][YBK_MAX_KEYS];

    uint8_t led_enabled;
    uint8_t led_mode;
    uint8_t led_brightness;
    uint8_t led_effect_speed;
    uint8_t led_color_r;
    uint8_t led_color_g;
    uint8_t led_color_b;
    uint8_t reserved;
} __attribute__((packed)) keyboard_profile_t;

esp_err_t keyboard_profile_init(void);
const keyboard_profile_t *keyboard_profile_get(void);
const keyboard_action_t *keyboard_profile_get_action(uint8_t layer, uint8_t key_num);
uint32_t keyboard_profile_get_checksum(void);

esp_err_t keyboard_profile_stage(const uint8_t *data, size_t len);
esp_err_t keyboard_profile_commit_staged(void);
esp_err_t keyboard_profile_reset_default(void);
esp_err_t keyboard_profile_save_active(void);

bool keyboard_profile_validate(const keyboard_profile_t *profile);
void keyboard_profile_set_default(keyboard_profile_t *profile);
void keyboard_profile_apply_lighting_runtime(void);
void keyboard_profile_preview_lighting(uint8_t mode, bool enabled, uint8_t brightness,
                                       uint8_t red, uint8_t green, uint8_t blue);

void keyboard_profile_set_led_enabled(bool enabled);
void keyboard_profile_adjust_brightness(int delta);
void keyboard_profile_next_led_mode(void);

#ifdef __cplusplus
}
#endif

#endif // _KEYBOARD_PROFILE_H_
