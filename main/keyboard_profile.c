#include "keyboard_profile.h"

#include <string.h>
#include <stddef.h>
#include "class/hid/hid_device.h"
#include "esp_log.h"
#include "nvs.h"
#include "sdkconfig.h"
#include "nvs_config.h"
#include "led.h"
#include "lamp_array/pixel.h"

static const char *TAG = "keyboard_profile";

#define PROFILE_NVS_KEY "profile2"
#define PROFILE_BRIGHTNESS_STEP 10

static keyboard_profile_t s_profile;
static keyboard_profile_t s_pending_profile;
static bool s_pending_valid = false;

static const keyboard_action_t s_none_action = {
    .type = YBK_ACTION_NONE,
    .flags = 0,
    .code = 0,
};

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

    profile->led_enabled = DEFAULT_LED_ENABLED;
    profile->led_mode = YBK_LED_MODE_HID;
    profile->led_brightness = DEFAULT_LED_BRIGHTNESS;
    profile->led_effect_speed = 50;
    profile->led_color_r = 255;
    profile->led_color_g = 255;
    profile->led_color_b = 255;

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

    if (profile->led_mode > YBK_LED_MODE_BLINK || profile->led_brightness > 100) {
        return false;
    }

    for (uint8_t layer = 0; layer < YBK_LAYER_COUNT; layer++) {
        for (uint8_t key = 0; key < YBK_MAX_KEYS; key++) {
            uint8_t type = profile->keymap[layer][key].type;
            if (type > YBK_ACTION_LED_EFFECT_NEXT) {
                return false;
            }
        }
    }

    return true;
}

static esp_err_t load_profile_from_nvs(keyboard_profile_t *profile)
{
#ifdef CONFIG_NVS_STORAGE_ENABLE
    nvs_handle_t handle;
    size_t size = sizeof(*profile);
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = nvs_get_blob(handle, PROFILE_NVS_KEY, profile, &size);
    nvs_close(handle);

    if (ret != ESP_OK) {
        return ret;
    }

    if (size != sizeof(*profile) || !keyboard_profile_validate(profile)) {
        return ESP_ERR_INVALID_CRC;
    }

    return ESP_OK;
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

static void apply_lighting_values(uint8_t mode, bool enabled, uint8_t brightness,
                                  uint8_t red, uint8_t green, uint8_t blue)
{
    led_state = enabled;
    led_brightness = brightness;
    led_effects = mode;

    LampColor color = make_scaled_color(brightness, red, green, blue);
    NeopixelEffect effect = HID;
    if (mode == YBK_LED_MODE_SOLID) {
        effect = SOLID;
    } else if (mode == YBK_LED_MODE_BLINK) {
        effect = BLINK;
    }
    NeopixelSetEffect(effect, color);
}

esp_err_t keyboard_profile_init(void)
{
    keyboard_profile_set_default(&s_profile);

    keyboard_profile_t loaded;
    esp_err_t ret = load_profile_from_nvs(&loaded);
    if (ret == ESP_OK) {
        s_profile = loaded;
        ESP_LOGI(TAG, "Profile loaded: checksum=0x%08lx", (unsigned long)s_profile.checksum);
    } else {
        ESP_LOGW(TAG, "Using default profile: %s", esp_err_to_name(ret));
        ESP_ERROR_CHECK_WITHOUT_ABORT(save_profile_to_nvs(&s_profile));
    }

    led_state = s_profile.led_enabled;
    led_brightness = s_profile.led_brightness;
    led_effects = s_profile.led_mode;
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

esp_err_t keyboard_profile_stage(const uint8_t *data, size_t len)
{
    if (data == NULL || len != sizeof(keyboard_profile_t)) {
        return ESP_ERR_INVALID_SIZE;
    }

    keyboard_profile_t candidate;
    memcpy(&candidate, data, sizeof(candidate));
    if (candidate.checksum == 0) {
        profile_fix_checksum(&candidate);
    }

    if (!keyboard_profile_validate(&candidate)) {
        return ESP_ERR_INVALID_ARG;
    }

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
        keyboard_profile_apply_lighting_runtime();
    }
    return ret;
}

esp_err_t keyboard_profile_reset_default(void)
{
    keyboard_profile_set_default(&s_profile);
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
    apply_lighting_values(s_profile.led_mode,
                          s_profile.led_enabled,
                          s_profile.led_brightness,
                          s_profile.led_color_r,
                          s_profile.led_color_g,
                          s_profile.led_color_b);
}

void keyboard_profile_preview_lighting(uint8_t mode, bool enabled, uint8_t brightness,
                                       uint8_t red, uint8_t green, uint8_t blue)
{
    if (mode > YBK_LED_MODE_BLINK) {
        mode = YBK_LED_MODE_HID;
    }
    if (brightness > 100) {
        brightness = 100;
    }
    apply_lighting_values(mode, enabled, brightness, red, green, blue);
}

void keyboard_profile_set_led_enabled(bool enabled)
{
    s_profile.led_enabled = enabled;
    keyboard_profile_apply_lighting_runtime();
    ESP_ERROR_CHECK_WITHOUT_ABORT(keyboard_profile_save_active());
}

void keyboard_profile_adjust_brightness(int delta)
{
    int brightness = s_profile.led_brightness + delta;
    if (brightness < 0) {
        brightness = 0;
    } else if (brightness > 100) {
        brightness = 100;
    }

    s_profile.led_brightness = (uint8_t)brightness;
    keyboard_profile_apply_lighting_runtime();
    ESP_ERROR_CHECK_WITHOUT_ABORT(keyboard_profile_save_active());
}

void keyboard_profile_next_led_mode(void)
{
    s_profile.led_mode++;
    if (s_profile.led_mode > YBK_LED_MODE_BLINK) {
        s_profile.led_mode = YBK_LED_MODE_HID;
    }

    keyboard_profile_apply_lighting_runtime();
    ESP_ERROR_CHECK_WITHOUT_ABORT(keyboard_profile_save_active());
}
