#ifndef _KEYBOARD_PROFILE_H_
#define _KEYBOARD_PROFILE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "keyboard_lighting_topology.h"

#ifdef __cplusplus
extern "C" {
#endif

#define YBK_PROFILE_MAGIC 0x504B4259u /* "YBKP" little-endian */
#define YBK_PROFILE_VERSION 8
#define YBK_MAX_KEYS 80
#define YBK_LAYER_COUNT 2
#define YBK_LIGHTING_PRESET_COUNT 6
#define YBK_LIGHTING_KEY_MASK_BYTES ((YBK_MAX_KEYS + 7) / 8)

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
    YBK_ACTION_POWER_MODE_NEXT = 9,
} keyboard_action_type_t;

typedef enum {
    YBK_LED_MODE_HID = 0,
    YBK_LED_MODE_SOLID = 1,
    YBK_LED_MODE_BREATH = 2,
    YBK_LED_MODE_RAINBOW = 3,
    YBK_LED_MODE_WAVE = 4,
    YBK_LED_MODE_GROUP_STATIC = 5,
    YBK_LED_MODE_PER_KEY_STATIC = 6,
    YBK_LED_MODE_KEY_FADE = 7,
    YBK_LED_MODE_RIPPLE = 8,
} keyboard_led_mode_t;

typedef enum {
    YBK_POWER_MODE_GAME = 0,
    YBK_POWER_MODE_OFFICE = 1,
    YBK_POWER_MODE_SAVER = 2,
} keyboard_power_mode_t;

typedef struct {
    uint8_t type;
    uint8_t flags;
    uint16_t code;
} __attribute__((packed)) keyboard_action_t;

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t brightness;
} __attribute__((packed)) keyboard_lighting_key_color_t;

typedef struct {
    uint8_t enabled;
    uint8_t mode;
    uint8_t brightness;
    uint8_t speed;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t reserved;
    uint8_t key_mask[YBK_LIGHTING_KEY_MASK_BYTES];
    keyboard_lighting_key_color_t per_key[YBK_MAX_KEYS];
} __attribute__((packed)) keyboard_lighting_preset_t;

typedef struct {
    uint8_t default_mode;
    uint8_t allow_mode_cycle;
    uint8_t scan_interval_game_ms;
    uint8_t scan_interval_office_ms;
    uint8_t scan_interval_saver_ms;
    uint8_t idle_scan_interval_ms;
    uint16_t idle_enter_low_power_ms;
    uint32_t idle_enter_deep_sleep_ms;
    uint8_t reserved[4];
} __attribute__((packed)) keyboard_power_profile_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint32_t checksum;

    keyboard_action_t keymap[YBK_LAYER_COUNT][YBK_MAX_KEYS];

    uint8_t lighting_active_preset;
    uint8_t lighting_reserved[3];
    keyboard_lighting_preset_t lighting_presets[YBK_LIGHTING_PRESET_COUNT];
    keyboard_power_profile_t power;
} __attribute__((packed)) keyboard_profile_t;

esp_err_t keyboard_profile_init(void);
const keyboard_profile_t *keyboard_profile_get(void);
const keyboard_action_t *keyboard_profile_get_action(uint8_t layer, uint8_t key_num);
uint32_t keyboard_profile_get_checksum(void);
const keyboard_power_profile_t *keyboard_profile_get_power_profile(void);
keyboard_power_mode_t keyboard_profile_get_power_mode(void);
uint8_t keyboard_profile_get_scan_interval_ms(void);
uint8_t keyboard_profile_get_idle_scan_interval_ms(void);
uint16_t keyboard_profile_get_idle_enter_low_power_ms(void);
uint32_t keyboard_profile_get_idle_enter_deep_sleep_ms(void);
bool keyboard_profile_socd_enabled(void);
uint8_t keyboard_profile_socd_delay_ms(void);
bool keyboard_profile_socd_randomize(void);
bool keyboard_profile_reverse_tap_enabled(void);
uint8_t keyboard_profile_reverse_tap_delay_ms(void);
uint8_t keyboard_profile_reverse_tap_duration_ms(void);
bool keyboard_profile_reverse_tap_randomize(void);
bool keyboard_profile_allow_power_mode_cycle(void);
bool keyboard_profile_cycle_power_mode(void);

esp_err_t keyboard_profile_stage(const uint8_t *data, size_t len);
esp_err_t keyboard_profile_commit_staged(void);
esp_err_t keyboard_profile_reset_default(void);
esp_err_t keyboard_profile_save_active(void);

bool keyboard_profile_validate(const keyboard_profile_t *profile);
void keyboard_profile_set_default(keyboard_profile_t *profile);
void keyboard_profile_apply_lighting_runtime(void);
void keyboard_profile_lighting_tick(uint32_t elapsed_ms);
void keyboard_profile_preview_lighting(uint8_t mode, bool enabled, uint8_t brightness, uint8_t speed,
                                       uint8_t red, uint8_t green, uint8_t blue);
void keyboard_profile_preview_lighting_preset(const keyboard_lighting_preset_t *preset);
void keyboard_profile_preview_led_index(uint16_t led_index, uint8_t brightness,
                                        uint8_t red, uint8_t green, uint8_t blue);
void keyboard_profile_note_pressed_keys(const int *pressed_pins, int num_pressed_pins);

void keyboard_profile_set_led_enabled(bool enabled);
void keyboard_profile_adjust_brightness(int delta);
void keyboard_profile_next_led_mode(void);

#ifdef __cplusplus
}
#endif

#endif // _KEYBOARD_PROFILE_H_
