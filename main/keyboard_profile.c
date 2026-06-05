#include "keyboard_profile.h"

#include <string.h>
#include <stddef.h>
#include "class/hid/hid_device.h"
#include "esp_log.h"
#include "nvs.h"
#include "sdkconfig.h"
#include "nvs_config.h"
#include "led.h"
#include "lightmap.h"
#include "lamp_array/pixel.h"

static const char *TAG = "keyboard_profile";

#define PROFILE_NVS_KEY "profile2"
#define PROFILE_BRIGHTNESS_STEP 10
#define PROFILE_EFFECT_SPEED_DEFAULT 50
#define PROFILE_EFFECT_SPEED_MAX 100
#define PROFILE_LIGHTING_CYCLE_INTERVAL_DEFAULT_SEC 8
#define PROFILE_LIGHTING_CYCLE_INTERVAL_MIN_SEC 1
#define PROFILE_LIGHTING_CYCLE_INTERVAL_MAX_SEC 120
#define PROFILE_SCAN_INTERVAL_MIN 1
#define PROFILE_SCAN_INTERVAL_MAX 100
#define PROFILE_IDLE_SCAN_INTERVAL_DEFAULT 100
#define PROFILE_SOCD_DELAY_DEFAULT 10
#define PROFILE_SOCD_DELAY_MAX 50
#define PROFILE_REVERSE_TAP_DURATION_DEFAULT 12
#define PROFILE_REVERSE_TAP_DURATION_MAX 50
#define PROFILE_POWER_FLAGS_SOCD_ENABLED (1u << 0)
#define PROFILE_POWER_FLAGS_SOCD_RANDOMIZE (1u << 1)
#define PROFILE_POWER_FLAGS_REVERSE_TAP_ENABLED (1u << 2)
#define PROFILE_POWER_FLAGS_REVERSE_TAP_RANDOMIZE (1u << 3)
#define PROFILE_POWER_RESERVED_FLAGS 0
#define PROFILE_POWER_RESERVED_SOCD_DELAY 1
#define PROFILE_POWER_RESERVED_REVERSE_TAP_DURATION 2
#define PROFILE_POWER_RESERVED_REVERSE_TAP_DELAY 3

static keyboard_profile_t s_profile;
static keyboard_profile_t s_pending_profile;
static bool s_pending_valid = false;
static keyboard_power_mode_t s_active_power_mode = YBK_POWER_MODE_OFFICE;
static bool s_lighting_cycle_paused = false;
static uint32_t s_lighting_cycle_elapsed_ms = 0;

static const keyboard_action_t s_none_action = {
    .type = YBK_ACTION_NONE,
    .flags = 0,
    .code = 0,
};

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
    keyboard_power_profile_t power;
} __attribute__((packed)) keyboard_profile_v3_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint32_t checksum;
    keyboard_action_t keymap[YBK_LAYER_COUNT][YBK_MAX_KEYS];
    uint8_t lighting_active_preset;
    uint8_t lighting_reserved[3];
    struct {
        uint8_t enabled;
        uint8_t mode;
        uint8_t brightness;
        uint8_t speed;
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t reserved;
    } lighting_presets[YBK_LIGHTING_PRESET_COUNT];
    keyboard_power_profile_t power;
} __attribute__((packed)) keyboard_profile_v4_t;

static uint8_t sanitize_active_lighting_preset(uint8_t active_index);
static void sanitize_lighting_preset(keyboard_lighting_preset_t *preset);

static keyboard_led_mode_t sanitize_lighting_mode(uint8_t mode)
{
    if (mode > YBK_LED_MODE_RIPPLE) {
        return YBK_LED_MODE_SOLID;
    }
    return (keyboard_led_mode_t)mode;
}

static uint8_t sanitize_lighting_cycle_interval_sec(uint8_t value)
{
    if (value < PROFILE_LIGHTING_CYCLE_INTERVAL_MIN_SEC ||
        value > PROFILE_LIGHTING_CYCLE_INTERVAL_MAX_SEC) {
        return PROFILE_LIGHTING_CYCLE_INTERVAL_DEFAULT_SEC;
    }
    return value;
}

static bool lighting_auto_cycle_enabled_for(const keyboard_profile_t *profile)
{
    return profile != NULL && profile->lighting_reserved[0] != 0;
}

static void sanitize_lighting_runtime_profile(keyboard_profile_t *profile)
{
    if (profile == NULL) {
        return;
    }

    profile->lighting_active_preset = sanitize_active_lighting_preset(profile->lighting_active_preset);
    profile->lighting_reserved[0] = lighting_auto_cycle_enabled_for(profile) ? 1 : 0;
    profile->lighting_reserved[1] = 0;
    profile->lighting_reserved[2] = 0;

    for (uint8_t i = 0; i < YBK_LIGHTING_PRESET_COUNT; i++) {
        sanitize_lighting_preset(&profile->lighting_presets[i]);
    }
}

static void lighting_preset_set(keyboard_lighting_preset_t *preset, bool enabled, uint8_t mode,
                                uint8_t brightness, uint8_t speed,
                                uint8_t red, uint8_t green, uint8_t blue)
{
    if (preset == NULL) {
        return;
    }

    preset->enabled = enabled ? 1 : 0;
    preset->mode = (uint8_t)sanitize_lighting_mode(mode);
    preset->brightness = brightness > 100 ? 100 : brightness;
    preset->speed = speed == 0 ? PROFILE_EFFECT_SPEED_DEFAULT :
                    (speed > PROFILE_EFFECT_SPEED_MAX ? PROFILE_EFFECT_SPEED_MAX : speed);
    preset->red = red;
    preset->green = green;
    preset->blue = blue;
    preset->reserved = PROFILE_LIGHTING_CYCLE_INTERVAL_DEFAULT_SEC;
    memset(preset->key_mask, 0, sizeof(preset->key_mask));
    memset(preset->per_key, 0, sizeof(preset->per_key));
}

static void sanitize_lighting_preset(keyboard_lighting_preset_t *preset)
{
    if (preset == NULL) {
        return;
    }

    preset->enabled = preset->enabled ? 1 : 0;
    preset->mode = (uint8_t)sanitize_lighting_mode(preset->mode);
    if (preset->brightness > 100) {
        preset->brightness = 100;
    }
    preset->speed = preset->speed == 0 ? PROFILE_EFFECT_SPEED_DEFAULT :
                    (preset->speed > PROFILE_EFFECT_SPEED_MAX ? PROFILE_EFFECT_SPEED_MAX : preset->speed);
    preset->reserved = sanitize_lighting_cycle_interval_sec(preset->reserved);
    for (uint8_t key = 0; key < YBK_MAX_KEYS; key++) {
        if (preset->per_key[key].brightness > 100) {
            preset->per_key[key].brightness = 100;
        }
    }
}

static uint8_t sanitize_active_lighting_preset(uint8_t active_index)
{
    if (active_index >= YBK_LIGHTING_PRESET_COUNT) {
        return 0;
    }
    return active_index;
}

static uint8_t enabled_lighting_preset_count(const keyboard_profile_t *profile)
{
    if (profile == NULL) {
        return 0;
    }

    uint8_t count = 0;
    for (uint8_t i = 0; i < YBK_LIGHTING_PRESET_COUNT; i++) {
        if (profile->lighting_presets[i].enabled) {
            count++;
        }
    }
    return count;
}

static bool key_mask_get(const uint8_t *mask, uint8_t key_num)
{
    if (mask == NULL || key_num >= YBK_MAX_KEYS) {
        return false;
    }

    return (mask[key_num / 8] & (uint8_t)(1u << (key_num % 8))) != 0;
}

static keyboard_power_mode_t sanitize_power_mode(uint8_t mode)
{
    if (mode > YBK_POWER_MODE_SAVER) {
        return YBK_POWER_MODE_OFFICE;
    }
    return (keyboard_power_mode_t)mode;
}

static uint8_t sanitize_scan_interval(uint8_t value, uint8_t fallback)
{
    if (value < PROFILE_SCAN_INTERVAL_MIN || value > PROFILE_SCAN_INTERVAL_MAX) {
        return fallback;
    }
    return value;
}

static uint8_t sanitize_socd_delay(uint8_t value)
{
    if (value > PROFILE_SOCD_DELAY_MAX) {
        return PROFILE_SOCD_DELAY_DEFAULT;
    }
    return value;
}

static uint8_t sanitize_reverse_tap_duration(uint8_t value)
{
    if (value > PROFILE_REVERSE_TAP_DURATION_MAX) {
        return PROFILE_REVERSE_TAP_DURATION_DEFAULT;
    }
    return value;
}

static void sanitize_power_profile(keyboard_power_profile_t *power)
{
    if (power == NULL) {
        return;
    }

    power->default_mode = (uint8_t)sanitize_power_mode(power->default_mode);
    power->allow_mode_cycle = power->allow_mode_cycle ? 1 : 0;
    power->scan_interval_game_ms = sanitize_scan_interval(power->scan_interval_game_ms, 1);
    power->scan_interval_office_ms = sanitize_scan_interval(power->scan_interval_office_ms, 4);
    power->scan_interval_saver_ms = sanitize_scan_interval(power->scan_interval_saver_ms, 8);
    power->idle_scan_interval_ms = sanitize_scan_interval(power->idle_scan_interval_ms,
                                                          PROFILE_IDLE_SCAN_INTERVAL_DEFAULT);
    power->reserved[PROFILE_POWER_RESERVED_SOCD_DELAY] =
        sanitize_socd_delay(power->reserved[PROFILE_POWER_RESERVED_SOCD_DELAY]);
    power->reserved[PROFILE_POWER_RESERVED_REVERSE_TAP_DURATION] =
        sanitize_reverse_tap_duration(power->reserved[PROFILE_POWER_RESERVED_REVERSE_TAP_DURATION]);
    power->reserved[PROFILE_POWER_RESERVED_REVERSE_TAP_DELAY] =
        sanitize_reverse_tap_duration(power->reserved[PROFILE_POWER_RESERVED_REVERSE_TAP_DELAY]);
    power->reserved[PROFILE_POWER_RESERVED_FLAGS] &= (PROFILE_POWER_FLAGS_SOCD_ENABLED |
                                                      PROFILE_POWER_FLAGS_SOCD_RANDOMIZE |
                                                      PROFILE_POWER_FLAGS_REVERSE_TAP_ENABLED |
                                                      PROFILE_POWER_FLAGS_REVERSE_TAP_RANDOMIZE);
}

static void reset_runtime_power_mode(void)
{
    s_active_power_mode = sanitize_power_mode(s_profile.power.default_mode);
}

static uint32_t checksum_update(uint32_t hash, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

static uint32_t calculate_profile_checksum(const keyboard_profile_t *profile)
{
    const uint8_t *data = (const uint8_t *)profile;
    const size_t checksum_offset = offsetof(keyboard_profile_t, checksum);
    uint32_t hash = 2166136261u;

    hash = checksum_update(hash, data, checksum_offset);
    hash = checksum_update(hash,
                           data + checksum_offset + sizeof(profile->checksum),
                           sizeof(*profile) - checksum_offset - sizeof(profile->checksum));
    return hash;
}

static void profile_fix_checksum(keyboard_profile_t *profile)
{
    profile->magic = YBK_PROFILE_MAGIC;
    profile->version = YBK_PROFILE_VERSION;
    profile->size = sizeof(keyboard_profile_t);
    profile->checksum = calculate_profile_checksum(profile);
}

static void set_action(keyboard_profile_t *profile, uint8_t layer, uint8_t key_num,
                       uint8_t type, uint16_t code)
{
    if (layer >= YBK_LAYER_COUNT || key_num == 0 || key_num >= YBK_MAX_KEYS) {
        return;
    }

    profile->keymap[layer][key_num].type = type;
    profile->keymap[layer][key_num].flags = 0;
    profile->keymap[layer][key_num].code = code;
}

static void set_action_if_empty(keyboard_profile_t *profile, uint8_t layer, uint8_t key_num,
                                uint8_t type, uint16_t code)
{
    if (layer >= YBK_LAYER_COUNT || key_num == 0 || key_num >= YBK_MAX_KEYS) {
        return;
    }

    if (profile->keymap[layer][key_num].type == YBK_ACTION_NONE) {
        set_action(profile, layer, key_num, type, code);
    }
}

static void set_key(keyboard_profile_t *profile, uint8_t key_num, uint16_t hid_key)
{
    set_action(profile, YBK_LAYER_BASE, key_num, YBK_ACTION_KEY, hid_key);
}

static void set_fn_key(keyboard_profile_t *profile, uint8_t key_num, uint16_t hid_key)
{
    set_action(profile, YBK_LAYER_FN, key_num, YBK_ACTION_KEY, hid_key);
}

static void set_modifier(keyboard_profile_t *profile, uint8_t key_num, uint16_t modifier)
{
    set_action(profile, YBK_LAYER_BASE, key_num, YBK_ACTION_MODIFIER, modifier);
}

static void set_consumer(keyboard_profile_t *profile, uint8_t layer, uint8_t key_num, uint16_t usage)
{
    set_action(profile, layer, key_num, YBK_ACTION_CONSUMER, usage);
}

void keyboard_profile_set_default(keyboard_profile_t *profile)
{
    memset(profile, 0, sizeof(*profile));

    set_key(profile, CONFIG_KEY_A_NUM, HID_KEY_A);
    set_key(profile, CONFIG_KEY_B_NUM, HID_KEY_B);
    set_key(profile, CONFIG_KEY_C_NUM, HID_KEY_C);
    set_key(profile, CONFIG_KEY_D_NUM, HID_KEY_D);
    set_key(profile, CONFIG_KEY_E_NUM, HID_KEY_E);
    set_key(profile, CONFIG_KEY_F_NUM, HID_KEY_F);
    set_key(profile, CONFIG_KEY_G_NUM, HID_KEY_G);
    set_key(profile, CONFIG_KEY_H_NUM, HID_KEY_H);
    set_key(profile, CONFIG_KEY_I_NUM, HID_KEY_I);
    set_key(profile, CONFIG_KEY_J_NUM, HID_KEY_J);
    set_key(profile, CONFIG_KEY_K_NUM, HID_KEY_K);
    set_key(profile, CONFIG_KEY_L_NUM, HID_KEY_L);
    set_key(profile, CONFIG_KEY_M_NUM, HID_KEY_M);
    set_key(profile, CONFIG_KEY_N_NUM, HID_KEY_N);
    set_key(profile, CONFIG_KEY_O_NUM, HID_KEY_O);
    set_key(profile, CONFIG_KEY_P_NUM, HID_KEY_P);
    set_key(profile, CONFIG_KEY_Q_NUM, HID_KEY_Q);
    set_key(profile, CONFIG_KEY_R_NUM, HID_KEY_R);
    set_key(profile, CONFIG_KEY_S_NUM, HID_KEY_S);
    set_key(profile, CONFIG_KEY_T_NUM, HID_KEY_T);
    set_key(profile, CONFIG_KEY_U_NUM, HID_KEY_U);
    set_key(profile, CONFIG_KEY_V_NUM, HID_KEY_V);
    set_key(profile, CONFIG_KEY_W_NUM, HID_KEY_W);
    set_key(profile, CONFIG_KEY_X_NUM, HID_KEY_X);
    set_key(profile, CONFIG_KEY_Y_NUM, HID_KEY_Y);
    set_key(profile, CONFIG_KEY_Z_NUM, HID_KEY_Z);
    set_key(profile, CONFIG_KEY_SPACE_NUM, HID_KEY_SPACE);
    set_key(profile, CONFIG_KEY_ENTER_NUM, HID_KEY_ENTER);
    set_key(profile, CONFIG_KEY_ESCAPE_NUM, HID_KEY_ESCAPE);
    set_key(profile, CONFIG_KEY_CAPS_LOCK_NUM, HID_KEY_CAPS_LOCK);
    set_key(profile, CONFIG_KEY_TAB_NUM, HID_KEY_TAB);
    set_key(profile, CONFIG_KEY_SEMICOLON_NUM, HID_KEY_SEMICOLON);
    set_key(profile, CONFIG_KEY_APOSTROPHE_NUM, HID_KEY_APOSTROPHE);
    set_key(profile, CONFIG_KEY_GRAVE_NUM, HID_KEY_GRAVE);
    set_key(profile, CONFIG_KEY_COMMA_NUM, HID_KEY_COMMA);
    set_key(profile, CONFIG_KEY_PERIOD_NUM, HID_KEY_PERIOD);
    set_key(profile, CONFIG_KEY_SLASH_NUM, HID_KEY_SLASH);
    set_key(profile, CONFIG_KEY_BACKSLASH_NUM, HID_KEY_BACKSLASH);
    set_key(profile, CONFIG_KEY_BRACKET_LEFT_NUM, HID_KEY_BRACKET_LEFT);
    set_key(profile, CONFIG_KEY_BRACKET_RIGHT_NUM, HID_KEY_BRACKET_RIGHT);
    set_key(profile, CONFIG_KEY_ARROW_LEFT_NUM, HID_KEY_ARROW_LEFT);
    set_key(profile, CONFIG_KEY_ARROW_DOWN_NUM, HID_KEY_ARROW_DOWN);
    set_key(profile, CONFIG_KEY_ARROW_RIGHT_NUM, HID_KEY_ARROW_RIGHT);
    set_key(profile, CONFIG_KEY_ARROW_UP_NUM, HID_KEY_ARROW_UP);
    set_key(profile, CONFIG_KEY_1_NUM, HID_KEY_1);
    set_key(profile, CONFIG_KEY_2_NUM, HID_KEY_2);
    set_key(profile, CONFIG_KEY_3_NUM, HID_KEY_3);
    set_key(profile, CONFIG_KEY_4_NUM, HID_KEY_4);
    set_key(profile, CONFIG_KEY_5_NUM, HID_KEY_5);
    set_key(profile, CONFIG_KEY_6_NUM, HID_KEY_6);
    set_key(profile, CONFIG_KEY_7_NUM, HID_KEY_7);
    set_key(profile, CONFIG_KEY_8_NUM, HID_KEY_8);
    set_key(profile, CONFIG_KEY_9_NUM, HID_KEY_9);
    set_key(profile, CONFIG_KEY_0_NUM, HID_KEY_0);
    set_key(profile, CONFIG_KEY_MINUS_NUM, HID_KEY_MINUS);
    set_key(profile, CONFIG_KEY_EQUAL_NUM, HID_KEY_EQUAL);
    set_key(profile, CONFIG_KEY_F1_NUM, HID_KEY_F1);
    set_key(profile, CONFIG_KEY_F2_NUM, HID_KEY_F2);
    set_key(profile, CONFIG_KEY_F3_NUM, HID_KEY_F3);
    set_key(profile, CONFIG_KEY_F4_NUM, HID_KEY_F4);
    set_key(profile, CONFIG_KEY_F5_NUM, HID_KEY_F5);
    set_key(profile, CONFIG_KEY_F6_NUM, HID_KEY_F6);
    set_key(profile, CONFIG_KEY_F7_NUM, HID_KEY_F7);
    set_key(profile, CONFIG_KEY_F8_NUM, HID_KEY_F8);
    set_key(profile, CONFIG_KEY_F9_NUM, HID_KEY_F9);
    set_key(profile, CONFIG_KEY_F10_NUM, HID_KEY_F10);
    set_key(profile, CONFIG_KEY_F11_NUM, HID_KEY_F11);
    set_key(profile, CONFIG_KEY_F12_NUM, HID_KEY_F12);
    set_key(profile, CONFIG_KEY_BACKSPACE_NUM, HID_KEY_BACKSPACE);
    set_key(profile, CONFIG_KEY_DELETE_NUM, HID_KEY_DELETE);
    set_key(profile, CONFIG_KEY_INSERT_NUM, HID_KEY_INSERT);

    set_modifier(profile, CONFIG_KEY_LEFT_CTRL_NUM, KEYBOARD_MODIFIER_LEFTCTRL);
    set_modifier(profile, CONFIG_KEY_LEFT_SHIFT_NUM, KEYBOARD_MODIFIER_LEFTSHIFT);
    set_modifier(profile, CONFIG_KEY_LEFT_ALT_NUM, KEYBOARD_MODIFIER_LEFTALT);
    set_modifier(profile, CONFIG_KEY_LEFT_GUI_NUM, KEYBOARD_MODIFIER_LEFTGUI);
    set_modifier(profile, CONFIG_KEY_RIGHT_CTRL_NUM, KEYBOARD_MODIFIER_RIGHTCTRL);
    set_modifier(profile, CONFIG_KEY_RIGHT_SHIFT_NUM, KEYBOARD_MODIFIER_RIGHTSHIFT);
    set_modifier(profile, CONFIG_KEY_RIGHT_ALT_NUM, KEYBOARD_MODIFIER_RIGHTALT);
    set_modifier(profile, CONFIG_KEY_RIGHT_GUI_NUM, KEYBOARD_MODIFIER_RIGHTGUI);

    set_action(profile, YBK_LAYER_BASE, CONFIG_KEY_FN_NUM, YBK_ACTION_LAYER_FN, 0);

    set_fn_key(profile, CONFIG_KEY_HOME_NUM, HID_KEY_HOME);
    set_fn_key(profile, CONFIG_KEY_PAGE_DOWN_NUM, HID_KEY_PAGE_DOWN);
    set_fn_key(profile, CONFIG_KEY_END_NUM, HID_KEY_END);
    set_fn_key(profile, CONFIG_KEY_PAGE_UP_NUM, HID_KEY_PAGE_UP);
    set_fn_key(profile, CONFIG_KEY_KEYPAD_1_NUM, HID_KEY_KEYPAD_1);
    set_fn_key(profile, CONFIG_KEY_KEYPAD_2_NUM, HID_KEY_KEYPAD_2);
    set_fn_key(profile, CONFIG_KEY_KEYPAD_3_NUM, HID_KEY_KEYPAD_3);
    set_fn_key(profile, CONFIG_KEY_KEYPAD_4_NUM, HID_KEY_KEYPAD_4);
    set_fn_key(profile, CONFIG_KEY_KEYPAD_5_NUM, HID_KEY_KEYPAD_5);
    set_fn_key(profile, CONFIG_KEY_KEYPAD_6_NUM, HID_KEY_KEYPAD_6);
    set_fn_key(profile, CONFIG_KEY_KEYPAD_7_NUM, HID_KEY_KEYPAD_7);
    set_fn_key(profile, CONFIG_KEY_KEYPAD_8_NUM, HID_KEY_KEYPAD_8);
    set_fn_key(profile, CONFIG_KEY_KEYPAD_9_NUM, HID_KEY_KEYPAD_9);
    set_fn_key(profile, CONFIG_KEY_KEYPAD_0_NUM, HID_KEY_KEYPAD_0);
    set_fn_key(profile, CONFIG_KEY_KEYPAD_SUBTRACT_NUM, HID_KEY_KEYPAD_SUBTRACT);
    set_fn_key(profile, CONFIG_KEY_KEYPAD_ADD_NUM, HID_KEY_KEYPAD_ADD);
    set_fn_key(profile, CONFIG_KEY_KEYPAD_DIVIDE_NUM, HID_KEY_KEYPAD_DIVIDE);

    set_consumer(profile, YBK_LAYER_FN, CONFIG_KEY_INSERT_NUM, HID_USAGE_CONSUMER_MUTE);
    set_consumer(profile, YBK_LAYER_FN, CONFIG_KEY_DELETE_NUM, HID_USAGE_CONSUMER_VOLUME_INCREMENT);
    set_consumer(profile, YBK_LAYER_FN, CONFIG_KEY_BACKSPACE_NUM, HID_USAGE_CONSUMER_VOLUME_DECREMENT);
    set_consumer(profile, YBK_LAYER_BASE, CONFIG_KEY_MUTE_BUTTON_NUM, HID_USAGE_CONSUMER_MUTE);
    set_consumer(profile, YBK_LAYER_BASE, CONFIG_KEY_VOLUME_UP_BUTTON_NUM, HID_USAGE_CONSUMER_VOLUME_INCREMENT);
    set_consumer(profile, YBK_LAYER_BASE, CONFIG_KEY_VOLUME_DOWN_BUTTON_NUM, HID_USAGE_CONSUMER_VOLUME_DECREMENT);

    set_action_if_empty(profile, YBK_LAYER_FN, CONFIG_KEY_LED_SWITCH_NUM, YBK_ACTION_LED_TOGGLE, 0);
    set_action_if_empty(profile, YBK_LAYER_FN, CONFIG_KEY_LED_ADD_NUM, YBK_ACTION_LED_BRIGHTNESS_UP, 0);
    set_action_if_empty(profile, YBK_LAYER_FN, CONFIG_KEY_LED_SUB_NUM, YBK_ACTION_LED_BRIGHTNESS_DOWN, 0);
    set_action_if_empty(profile, YBK_LAYER_FN, CONFIG_KEY_LED_EFFECT_NUM, YBK_ACTION_LED_EFFECT_NEXT, 0);
    set_action_if_empty(profile, YBK_LAYER_FN, CONFIG_KEY_ESCAPE_NUM, YBK_ACTION_POWER_MODE_NEXT, 0);

    profile->lighting_active_preset = 0;
    memset(profile->lighting_reserved, 0, sizeof(profile->lighting_reserved));
    lighting_preset_set(&profile->lighting_presets[0], false, YBK_LED_MODE_SOLID, 100, 50, 255, 255, 255);
    lighting_preset_set(&profile->lighting_presets[1], true, YBK_LED_MODE_SOLID, 100, 50, 255, 255, 255);
    lighting_preset_set(&profile->lighting_presets[2], true, YBK_LED_MODE_BREATH, 80, 35, 80, 220, 255);
    lighting_preset_set(&profile->lighting_presets[3], false, YBK_LED_MODE_WAVE, 100, 35, 80, 120, 255);
    lighting_preset_set(&profile->lighting_presets[4], false, YBK_LED_MODE_RAINBOW, 100, 40, 255, 255, 255);
    lighting_preset_set(&profile->lighting_presets[5], false, YBK_LED_MODE_HID, 100, 50, 255, 255, 255);
    profile->power.default_mode = YBK_POWER_MODE_OFFICE;
    profile->power.allow_mode_cycle = 1;
    profile->power.scan_interval_game_ms = 1;
    profile->power.scan_interval_office_ms = 4;
    profile->power.scan_interval_saver_ms = 8;
    profile->power.idle_scan_interval_ms = PROFILE_IDLE_SCAN_INTERVAL_DEFAULT;
    profile->power.idle_enter_low_power_ms = 15000;
    profile->power.idle_enter_deep_sleep_ms = 180000;
    memset(profile->power.reserved, 0, sizeof(profile->power.reserved));
    profile->power.reserved[PROFILE_POWER_RESERVED_SOCD_DELAY] = PROFILE_SOCD_DELAY_DEFAULT;
    profile->power.reserved[PROFILE_POWER_RESERVED_REVERSE_TAP_DURATION] =
        PROFILE_REVERSE_TAP_DURATION_DEFAULT;
    profile->power.reserved[PROFILE_POWER_RESERVED_REVERSE_TAP_DELAY] = 0;
    sanitize_power_profile(&profile->power);

    profile_fix_checksum(profile);
}

bool keyboard_profile_validate(const keyboard_profile_t *profile)
{
    if (profile == NULL) {
        return false;
    }

    if (profile->magic != YBK_PROFILE_MAGIC ||
        profile->version != YBK_PROFILE_VERSION ||
        profile->size != sizeof(keyboard_profile_t)) {
        return false;
    }

    if (profile->checksum != 0 && profile->checksum != calculate_profile_checksum(profile)) {
        return false;
    }

    if (profile->lighting_active_preset >= YBK_LIGHTING_PRESET_COUNT) {
        return false;
    }
    if (profile->lighting_reserved[0] > 1) {
        return false;
    }
    for (uint8_t i = 0; i < YBK_LIGHTING_PRESET_COUNT; i++) {
        const keyboard_lighting_preset_t *preset = &profile->lighting_presets[i];
        if (preset->mode > YBK_LED_MODE_RIPPLE ||
            preset->brightness > 100 ||
            preset->speed > PROFILE_EFFECT_SPEED_MAX ||
            (preset->reserved != 0 &&
             (preset->reserved < PROFILE_LIGHTING_CYCLE_INTERVAL_MIN_SEC ||
              preset->reserved > PROFILE_LIGHTING_CYCLE_INTERVAL_MAX_SEC))) {
            return false;
        }
        for (uint8_t key = 0; key < YBK_MAX_KEYS; key++) {
            if (preset->per_key[key].brightness > 100) {
                return false;
            }
        }
    }

    if (profile->power.default_mode > YBK_POWER_MODE_SAVER ||
        profile->power.allow_mode_cycle > 1) {
        return false;
    }
    if (profile->power.scan_interval_game_ms < PROFILE_SCAN_INTERVAL_MIN ||
        profile->power.scan_interval_game_ms > PROFILE_SCAN_INTERVAL_MAX ||
        profile->power.scan_interval_office_ms < PROFILE_SCAN_INTERVAL_MIN ||
        profile->power.scan_interval_office_ms > PROFILE_SCAN_INTERVAL_MAX ||
        profile->power.scan_interval_saver_ms < PROFILE_SCAN_INTERVAL_MIN ||
        profile->power.scan_interval_saver_ms > PROFILE_SCAN_INTERVAL_MAX ||
        profile->power.idle_scan_interval_ms < PROFILE_SCAN_INTERVAL_MIN ||
        profile->power.idle_scan_interval_ms > PROFILE_SCAN_INTERVAL_MAX ||
        (profile->power.reserved[PROFILE_POWER_RESERVED_FLAGS] &
         ~(PROFILE_POWER_FLAGS_SOCD_ENABLED |
           PROFILE_POWER_FLAGS_SOCD_RANDOMIZE |
           PROFILE_POWER_FLAGS_REVERSE_TAP_ENABLED |
           PROFILE_POWER_FLAGS_REVERSE_TAP_RANDOMIZE)) != 0 ||
        profile->power.reserved[PROFILE_POWER_RESERVED_SOCD_DELAY] > PROFILE_SOCD_DELAY_MAX ||
        profile->power.reserved[PROFILE_POWER_RESERVED_REVERSE_TAP_DURATION] > PROFILE_REVERSE_TAP_DURATION_MAX ||
        profile->power.reserved[PROFILE_POWER_RESERVED_REVERSE_TAP_DELAY] > PROFILE_REVERSE_TAP_DURATION_MAX) {
        return false;
    }

    bool has_input_action = false;
    for (uint8_t layer = 0; layer < YBK_LAYER_COUNT; layer++) {
        for (uint8_t key = 0; key < YBK_MAX_KEYS; key++) {
            uint8_t type = profile->keymap[layer][key].type;
            if (type > YBK_ACTION_POWER_MODE_NEXT) {
                return false;
            }
            if (type == YBK_ACTION_KEY ||
                type == YBK_ACTION_MODIFIER ||
                type == YBK_ACTION_LAYER_FN) {
                has_input_action = true;
            }
        }
    }

    return has_input_action;
}

static esp_err_t load_profile_from_nvs(keyboard_profile_t *profile)
{
#ifdef CONFIG_NVS_STORAGE_ENABLE
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        return ret;
    }

    size_t size = 0;
    ret = nvs_get_blob(handle, PROFILE_NVS_KEY, NULL, &size);
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }

    if (size == sizeof(*profile)) {
        keyboard_profile_t stored = {0};
        ret = nvs_get_blob(handle, PROFILE_NVS_KEY, &stored, &size);
        nvs_close(handle);

        if (ret != ESP_OK) {
            return ret;
        }

        if (stored.magic != YBK_PROFILE_MAGIC || stored.size != sizeof(stored)) {
            return ESP_ERR_INVALID_CRC;
        }

        if (stored.checksum != 0 && stored.checksum != calculate_profile_checksum(&stored)) {
            return ESP_ERR_INVALID_CRC;
        }

        if (stored.version == YBK_PROFILE_VERSION) {
            if (!keyboard_profile_validate(&stored)) {
                return ESP_ERR_INVALID_CRC;
            }
            sanitize_power_profile(&stored.power);
            sanitize_lighting_runtime_profile(&stored);
            *profile = stored;
            return ESP_OK;
        }

        if (stored.version == 7 || stored.version == 6 || stored.version == 5) {
            keyboard_profile_t migrated = stored;
            migrated.version = YBK_PROFILE_VERSION;
            uint8_t old_socd_enabled = migrated.power.reserved[0];
            uint8_t old_socd_delay = migrated.power.reserved[1];
            uint8_t old_socd_randomize = migrated.power.reserved[2];
            memset(migrated.power.reserved, 0, sizeof(migrated.power.reserved));
            if (old_socd_enabled) {
                migrated.power.reserved[PROFILE_POWER_RESERVED_FLAGS] |= PROFILE_POWER_FLAGS_SOCD_ENABLED;
            }
            if (old_socd_randomize) {
                migrated.power.reserved[PROFILE_POWER_RESERVED_FLAGS] |= PROFILE_POWER_FLAGS_SOCD_RANDOMIZE;
            }
            migrated.power.reserved[PROFILE_POWER_RESERVED_SOCD_DELAY] = old_socd_delay;
            migrated.power.reserved[PROFILE_POWER_RESERVED_REVERSE_TAP_DURATION] =
                PROFILE_REVERSE_TAP_DURATION_DEFAULT;
            migrated.power.reserved[PROFILE_POWER_RESERVED_REVERSE_TAP_DELAY] = 0;
            if (stored.version == 5) {
                migrated.lighting_reserved[0] = 0;
            }
            sanitize_power_profile(&migrated.power);
            sanitize_lighting_runtime_profile(&migrated);
            profile_fix_checksum(&migrated);
            *profile = migrated;
            return ESP_OK;
        }

        return ESP_ERR_INVALID_CRC;
    }

    if (size == sizeof(keyboard_profile_v4_t)) {
        keyboard_profile_v4_t legacy = {0};
        ret = nvs_get_blob(handle, PROFILE_NVS_KEY, &legacy, &size);
        nvs_close(handle);
        if (ret != ESP_OK) {
            return ret;
        }

        if (legacy.magic != YBK_PROFILE_MAGIC ||
            legacy.version != 4 ||
            legacy.size != sizeof(legacy)) {
            return ESP_ERR_INVALID_CRC;
        }

        keyboard_profile_t migrated = {0};
        keyboard_profile_set_default(&migrated);
        memcpy(migrated.keymap, legacy.keymap, sizeof(migrated.keymap));
        migrated.power = legacy.power;
        migrated.lighting_active_preset = sanitize_active_lighting_preset(legacy.lighting_active_preset);
        memset(migrated.lighting_reserved, 0, sizeof(migrated.lighting_reserved));
        for (uint8_t i = 0; i < YBK_LIGHTING_PRESET_COUNT; i++) {
            lighting_preset_set(&migrated.lighting_presets[i],
                                legacy.lighting_presets[i].enabled != 0,
                                legacy.lighting_presets[i].mode,
                                legacy.lighting_presets[i].brightness,
                                legacy.lighting_presets[i].speed,
                                legacy.lighting_presets[i].red,
                                legacy.lighting_presets[i].green,
                                legacy.lighting_presets[i].blue);
        }
        sanitize_power_profile(&migrated.power);
        sanitize_lighting_runtime_profile(&migrated);
        profile_fix_checksum(&migrated);
        *profile = migrated;
        return ESP_OK;
    }

    if (size == sizeof(keyboard_profile_v3_t)) {
        keyboard_profile_v3_t legacy = {0};
        ret = nvs_get_blob(handle, PROFILE_NVS_KEY, &legacy, &size);
        nvs_close(handle);
        if (ret != ESP_OK) {
            return ret;
        }

        if (legacy.magic != YBK_PROFILE_MAGIC ||
            legacy.version != 3 ||
            legacy.size != sizeof(legacy)) {
            return ESP_ERR_INVALID_CRC;
        }

        keyboard_profile_t migrated = {0};
        keyboard_profile_set_default(&migrated);
        memcpy(migrated.keymap, legacy.keymap, sizeof(migrated.keymap));
        migrated.power = legacy.power;
        memset(migrated.lighting_reserved, 0, sizeof(migrated.lighting_reserved));
        lighting_preset_set(&migrated.lighting_presets[0],
                            legacy.led_enabled != 0,
                            legacy.led_mode,
                            legacy.led_brightness,
                            legacy.led_effect_speed,
                            legacy.led_color_r,
                            legacy.led_color_g,
                            legacy.led_color_b);
        migrated.lighting_active_preset = 0;
        sanitize_power_profile(&migrated.power);
        sanitize_lighting_runtime_profile(&migrated);
        profile_fix_checksum(&migrated);
        *profile = migrated;
        return ESP_OK;
    }

    nvs_close(handle);
    return ESP_ERR_INVALID_SIZE;
#else
    (void)profile;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static esp_err_t save_profile_to_nvs(const keyboard_profile_t *profile)
{
#ifdef CONFIG_NVS_STORAGE_ENABLE
    nvs_handle_t handle;
    keyboard_profile_t copy = *profile;
    profile_fix_checksum(&copy);

    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = nvs_set_blob(handle, PROFILE_NVS_KEY, &copy, sizeof(copy));
    if (ret == ESP_OK) {
        ret = nvs_commit(handle);
    }
    nvs_close(handle);
    return ret;
#else
    (void)profile;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static LampColor make_scaled_color(uint8_t brightness, uint8_t red, uint8_t green, uint8_t blue)
{
    LampColor color = {
        .red = (uint8_t)(((uint32_t)red * brightness) / 100),
        .green = (uint8_t)(((uint32_t)green * brightness) / 100),
        .blue = (uint8_t)(((uint32_t)blue * brightness) / 100),
        .intensity = 0,
    };
    return color;
}

static void build_static_lamp_colors(const keyboard_lighting_preset_t *preset, LampColor *lamp_colors,
                                     uint32_t lamp_count)
{
    if (preset == NULL || lamp_colors == NULL) {
        return;
    }

    memset(lamp_colors, 0, lamp_count * sizeof(*lamp_colors));

    for (uint32_t lamp = 0; lamp < lamp_count; lamp++) {
        uint8_t key_num = LED_to_Key_Map[lamp];
        if (key_num == 0xFF || key_num >= YBK_MAX_KEYS) {
            continue;
        }

        if (preset->mode == YBK_LED_MODE_GROUP_STATIC) {
            if (!key_mask_get(preset->key_mask, key_num)) {
                continue;
            }
            lamp_colors[lamp] = make_scaled_color(preset->brightness, preset->red, preset->green, preset->blue);
            continue;
        }

        if (preset->mode == YBK_LED_MODE_PER_KEY_STATIC) {
            const keyboard_lighting_key_color_t *key_color = &preset->per_key[key_num];
            if (key_color->brightness == 0) {
                continue;
            }
            lamp_colors[lamp] = make_scaled_color(key_color->brightness,
                                                  key_color->red,
                                                  key_color->green,
                                                  key_color->blue);
        }
    }
}

static uint8_t normalize_effect_speed(uint8_t speed)
{
    if (speed == 0) {
        return PROFILE_EFFECT_SPEED_DEFAULT;
    }
    if (speed > PROFILE_EFFECT_SPEED_MAX) {
        return PROFILE_EFFECT_SPEED_MAX;
    }
    return speed;
}

static keyboard_lighting_preset_t *active_lighting_preset_mutable(void)
{
    return &s_profile.lighting_presets[sanitize_active_lighting_preset(s_profile.lighting_active_preset)];
}

static const keyboard_lighting_preset_t *active_lighting_preset(void)
{
    return &s_profile.lighting_presets[sanitize_active_lighting_preset(s_profile.lighting_active_preset)];
}

static bool find_next_enabled_lighting_preset(uint8_t *index_out)
{
    if (index_out == NULL) {
        return false;
    }

    uint8_t current = sanitize_active_lighting_preset(s_profile.lighting_active_preset);

    for (uint8_t offset = 1; offset <= YBK_LIGHTING_PRESET_COUNT; offset++) {
        uint8_t candidate = (uint8_t)((current + offset) % YBK_LIGHTING_PRESET_COUNT);
        if (s_profile.lighting_presets[candidate].enabled) {
            *index_out = candidate;
            return true;
        }
    }

    return false;
}

static void apply_lighting_values(uint8_t mode, bool enabled, uint8_t brightness, uint8_t speed,
                                  uint8_t red, uint8_t green, uint8_t blue,
                                  const keyboard_lighting_preset_t *preset)
{
    led_state = enabled;
    led_brightness = brightness;
    led_effects = mode;

    LampColor color = make_scaled_color(brightness, red, green, blue);
    NeopixelEffect effect = HID;
    if (mode == YBK_LED_MODE_SOLID) {
        effect = SOLID;
    } else if (mode == YBK_LED_MODE_BREATH) {
        effect = BREATH;
    } else if (mode == YBK_LED_MODE_RAINBOW) {
        effect = RAINBOW;
    } else if (mode == YBK_LED_MODE_WAVE) {
        effect = WAVE;
    } else if (mode == YBK_LED_MODE_KEY_FADE) {
        effect = REACTIVE_FADE;
    } else if (mode == YBK_LED_MODE_RIPPLE) {
        effect = RIPPLE;
    } else if (mode == YBK_LED_MODE_GROUP_STATIC || mode == YBK_LED_MODE_PER_KEY_STATIC) {
        effect = STATIC_MAP;
    }
    NeopixelSetEffect(effect, color, normalize_effect_speed(speed));
    if (effect == STATIC_MAP && preset != NULL) {
        LampColor lamp_colors[LIGHTMAP_NUM];
        build_static_lamp_colors(preset, lamp_colors, LIGHTMAP_NUM);
        NeopixelSetCustomColors(lamp_colors, LIGHTMAP_NUM);
    }
}

esp_err_t keyboard_profile_init(void)
{
    keyboard_profile_set_default(&s_profile);

    keyboard_profile_t loaded;
    esp_err_t ret = load_profile_from_nvs(&loaded);
    if (ret == ESP_OK) {
        s_profile = loaded;
        sanitize_power_profile(&s_profile.power);
        sanitize_lighting_runtime_profile(&s_profile);
        ESP_LOGI(TAG, "Profile loaded: checksum=0x%08lx", (unsigned long)s_profile.checksum);
    } else {
        ESP_LOGW(TAG, "Using default profile: %s", esp_err_to_name(ret));
        ESP_ERROR_CHECK_WITHOUT_ABORT(save_profile_to_nvs(&s_profile));
    }

    reset_runtime_power_mode();
    keyboard_profile_apply_lighting_runtime();
    return ESP_OK;
}

const keyboard_profile_t *keyboard_profile_get(void)
{
    return &s_profile;
}

const keyboard_action_t *keyboard_profile_get_action(uint8_t layer, uint8_t key_num)
{
    if (layer >= YBK_LAYER_COUNT || key_num >= YBK_MAX_KEYS) {
        return &s_none_action;
    }
    return &s_profile.keymap[layer][key_num];
}

uint32_t keyboard_profile_get_checksum(void)
{
    return s_profile.checksum;
}

const keyboard_power_profile_t *keyboard_profile_get_power_profile(void)
{
    return &s_profile.power;
}

keyboard_power_mode_t keyboard_profile_get_power_mode(void)
{
    return s_active_power_mode;
}

uint8_t keyboard_profile_get_scan_interval_ms(void)
{
    switch (s_active_power_mode) {
    case YBK_POWER_MODE_GAME:
        return s_profile.power.scan_interval_game_ms;
    case YBK_POWER_MODE_SAVER:
        return s_profile.power.scan_interval_saver_ms;
    case YBK_POWER_MODE_OFFICE:
    default:
        return s_profile.power.scan_interval_office_ms;
    }
}

uint8_t keyboard_profile_get_idle_scan_interval_ms(void)
{
    return s_profile.power.idle_scan_interval_ms;
}

uint16_t keyboard_profile_get_idle_enter_low_power_ms(void)
{
    return s_profile.power.idle_enter_low_power_ms;
}

uint32_t keyboard_profile_get_idle_enter_deep_sleep_ms(void)
{
    return s_profile.power.idle_enter_deep_sleep_ms;
}

bool keyboard_profile_socd_enabled(void)
{
    return (s_profile.power.reserved[PROFILE_POWER_RESERVED_FLAGS] & PROFILE_POWER_FLAGS_SOCD_ENABLED) != 0;
}

uint8_t keyboard_profile_socd_delay_ms(void)
{
    return sanitize_socd_delay(s_profile.power.reserved[PROFILE_POWER_RESERVED_SOCD_DELAY]);
}

bool keyboard_profile_socd_randomize(void)
{
    return (s_profile.power.reserved[PROFILE_POWER_RESERVED_FLAGS] & PROFILE_POWER_FLAGS_SOCD_RANDOMIZE) != 0;
}

bool keyboard_profile_reverse_tap_enabled(void)
{
    return (s_profile.power.reserved[PROFILE_POWER_RESERVED_FLAGS] &
            PROFILE_POWER_FLAGS_REVERSE_TAP_ENABLED) != 0;
}

uint8_t keyboard_profile_reverse_tap_delay_ms(void)
{
    return sanitize_reverse_tap_duration(
        s_profile.power.reserved[PROFILE_POWER_RESERVED_REVERSE_TAP_DELAY]);
}

uint8_t keyboard_profile_reverse_tap_duration_ms(void)
{
    return sanitize_reverse_tap_duration(
        s_profile.power.reserved[PROFILE_POWER_RESERVED_REVERSE_TAP_DURATION]);
}

bool keyboard_profile_reverse_tap_randomize(void)
{
    return (s_profile.power.reserved[PROFILE_POWER_RESERVED_FLAGS] &
            PROFILE_POWER_FLAGS_REVERSE_TAP_RANDOMIZE) != 0;
}

bool keyboard_profile_allow_power_mode_cycle(void)
{
    return s_profile.power.allow_mode_cycle != 0;
}

bool keyboard_profile_cycle_power_mode(void)
{
    if (!keyboard_profile_allow_power_mode_cycle()) {
        return false;
    }

    switch (s_active_power_mode) {
    case YBK_POWER_MODE_GAME:
        s_active_power_mode = YBK_POWER_MODE_OFFICE;
        break;
    case YBK_POWER_MODE_OFFICE:
        s_active_power_mode = YBK_POWER_MODE_SAVER;
        break;
    case YBK_POWER_MODE_SAVER:
    default:
        s_active_power_mode = YBK_POWER_MODE_GAME;
        break;
    }

    return true;
}

esp_err_t keyboard_profile_stage(const uint8_t *data, size_t len)
{
    if (data == NULL || len != sizeof(keyboard_profile_t)) {
        return ESP_ERR_INVALID_SIZE;
    }

    keyboard_profile_t candidate;
    memcpy(&candidate, data, sizeof(candidate));
    uint16_t original_version = candidate.version;
    if (original_version == 7 || original_version == 6 || original_version == 5) {
        uint8_t old_socd_enabled = candidate.power.reserved[0];
        uint8_t old_socd_delay = candidate.power.reserved[1];
        uint8_t old_socd_randomize = candidate.power.reserved[2];
        candidate.version = YBK_PROFILE_VERSION;
        memset(candidate.power.reserved, 0, sizeof(candidate.power.reserved));
        if (old_socd_enabled) {
            candidate.power.reserved[PROFILE_POWER_RESERVED_FLAGS] |= PROFILE_POWER_FLAGS_SOCD_ENABLED;
        }
        if (old_socd_randomize) {
            candidate.power.reserved[PROFILE_POWER_RESERVED_FLAGS] |= PROFILE_POWER_FLAGS_SOCD_RANDOMIZE;
        }
        candidate.power.reserved[PROFILE_POWER_RESERVED_SOCD_DELAY] = old_socd_delay;
        candidate.power.reserved[PROFILE_POWER_RESERVED_REVERSE_TAP_DURATION] =
            PROFILE_REVERSE_TAP_DURATION_DEFAULT;
        candidate.power.reserved[PROFILE_POWER_RESERVED_REVERSE_TAP_DELAY] = 0;
        if (original_version == 5) {
            candidate.lighting_reserved[0] = 0;
            candidate.lighting_reserved[1] = 0;
            candidate.lighting_reserved[2] = 0;
        }
        candidate.checksum = 0;
    }
    if (candidate.checksum == 0) {
        profile_fix_checksum(&candidate);
    }

    if (!keyboard_profile_validate(&candidate)) {
        return ESP_ERR_INVALID_ARG;
    }

    sanitize_power_profile(&candidate.power);
    sanitize_lighting_runtime_profile(&candidate);
    profile_fix_checksum(&candidate);
    s_pending_profile = candidate;
    s_pending_valid = true;
    return ESP_OK;
}

esp_err_t keyboard_profile_commit_staged(void)
{
    if (!s_pending_valid) {
        return ESP_ERR_INVALID_STATE;
    }

    s_profile = s_pending_profile;
    esp_err_t ret = save_profile_to_nvs(&s_profile);
    if (ret == ESP_OK) {
        s_pending_valid = false;
        reset_runtime_power_mode();
        s_lighting_cycle_paused = false;
        keyboard_profile_apply_lighting_runtime();
    }
    return ret;
}

esp_err_t keyboard_profile_reset_default(void)
{
    keyboard_profile_set_default(&s_profile);
    reset_runtime_power_mode();
    s_lighting_cycle_paused = false;
    keyboard_profile_apply_lighting_runtime();
    return save_profile_to_nvs(&s_profile);
}

esp_err_t keyboard_profile_save_active(void)
{
    profile_fix_checksum(&s_profile);
    return save_profile_to_nvs(&s_profile);
}

void keyboard_profile_apply_lighting_runtime(void)
{
    const keyboard_lighting_preset_t *preset = active_lighting_preset();
    s_lighting_cycle_elapsed_ms = 0;
    apply_lighting_values(preset->mode,
                          preset->enabled != 0,
                          preset->brightness,
                          preset->speed,
                          preset->red,
                          preset->green,
                          preset->blue,
                          preset);
}

void keyboard_profile_lighting_tick(uint32_t elapsed_ms)
{
    if (elapsed_ms == 0 ||
        !lighting_auto_cycle_enabled_for(&s_profile) ||
        s_lighting_cycle_paused ||
        enabled_lighting_preset_count(&s_profile) < 2) {
        return;
    }

    const keyboard_lighting_preset_t *preset = active_lighting_preset();
    uint32_t interval_ms = (uint32_t)sanitize_lighting_cycle_interval_sec(preset->reserved) * 1000u;
    if (interval_ms == 0) {
        return;
    }

    s_lighting_cycle_elapsed_ms += elapsed_ms;
    if (s_lighting_cycle_elapsed_ms < interval_ms) {
        return;
    }

    uint8_t next_preset = 0;
    if (!find_next_enabled_lighting_preset(&next_preset) || next_preset == s_profile.lighting_active_preset) {
        s_lighting_cycle_elapsed_ms = 0;
        return;
    }

    s_profile.lighting_active_preset = next_preset;
    keyboard_profile_apply_lighting_runtime();
}

void keyboard_profile_preview_lighting(uint8_t mode, bool enabled, uint8_t brightness, uint8_t speed,
                                       uint8_t red, uint8_t green, uint8_t blue)
{
    if (mode > YBK_LED_MODE_RIPPLE) {
        mode = YBK_LED_MODE_HID;
    }
    if (brightness > 100) {
        brightness = 100;
    }
    apply_lighting_values(mode, enabled, brightness, speed, red, green, blue, NULL);
}

void keyboard_profile_preview_lighting_preset(const keyboard_lighting_preset_t *preset)
{
    if (preset == NULL) {
        return;
    }

    keyboard_lighting_preset_t preview = *preset;
    sanitize_lighting_preset(&preview);
    apply_lighting_values(preview.mode,
                          preview.enabled != 0,
                          preview.brightness,
                          preview.speed,
                          preview.red,
                          preview.green,
                          preview.blue,
                          &preview);
}

void keyboard_profile_note_pressed_keys(const int *pressed_pins, int num_pressed_pins)
{
    uint8_t count = 0;

    if (pressed_pins != NULL && num_pressed_pins > 0) {
        count = (uint8_t)((num_pressed_pins > 255) ? 255 : num_pressed_pins);
    }

    NeopixelNotifyPressedKeys(pressed_pins, count);
}

void keyboard_profile_set_led_enabled(bool enabled)
{
    keyboard_lighting_preset_t *preset = active_lighting_preset_mutable();
    preset->enabled = enabled ? 1 : 0;
    keyboard_profile_apply_lighting_runtime();
    ESP_ERROR_CHECK_WITHOUT_ABORT(keyboard_profile_save_active());
}

void keyboard_profile_adjust_brightness(int delta)
{
    keyboard_lighting_preset_t *preset = active_lighting_preset_mutable();
    int brightness = preset->brightness + delta;
    if (brightness < 0) {
        brightness = 0;
    } else if (brightness > 100) {
        brightness = 100;
    }

    preset->brightness = (uint8_t)brightness;
    keyboard_profile_apply_lighting_runtime();
    ESP_ERROR_CHECK_WITHOUT_ABORT(keyboard_profile_save_active());
}

void keyboard_profile_next_led_mode(void)
{
    if (lighting_auto_cycle_enabled_for(&s_profile)) {
        s_lighting_cycle_paused = !s_lighting_cycle_paused;
        if (!s_lighting_cycle_paused) {
            s_lighting_cycle_elapsed_ms = 0;
        }
        return;
    }

    uint8_t next_preset = 0;
    if (!find_next_enabled_lighting_preset(&next_preset)) {
        return;
    }

    s_profile.lighting_active_preset = next_preset;
    keyboard_profile_apply_lighting_runtime();
    ESP_ERROR_CHECK_WITHOUT_ABORT(keyboard_profile_save_active());
}
