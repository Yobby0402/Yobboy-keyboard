#include "key_bind.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "class/hid/hid_device.h"
#include "usb_descriptors.h"
#include "keyboard_profile.h"
#include "esp_log.h"

static const char *TAG = "key_bind";

static TickType_t last_led_switch_time = 0;
static TickType_t last_volume_time = 0;

#define LED_CONTROL_COOLDOWN_MS CONFIG_LED_SWITCH_SLEEP
#define VOLUME_CONTROL_COOLDOWN_MS 200

static void send_consumer_control(uint16_t usage_code)
{
    uint8_t report[2] = {(uint8_t)(usage_code & 0xFF), (uint8_t)(usage_code >> 8)};
    tud_hid_report(REPORT_ID_CONSUMER_CONTROL, report, 2);
    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t empty_report[2] = {0, 0};
    tud_hid_report(REPORT_ID_CONSUMER_CONTROL, empty_report, 2);
}

const basic_key_mapping_t basic_key_mappings[] = {
    {CONFIG_KEY_A_NUM, HID_KEY_A},
    {CONFIG_KEY_B_NUM, HID_KEY_B},
    {CONFIG_KEY_C_NUM, HID_KEY_C},
    {CONFIG_KEY_D_NUM, HID_KEY_D},
    {CONFIG_KEY_E_NUM, HID_KEY_E},
    {CONFIG_KEY_F_NUM, HID_KEY_F},
    {CONFIG_KEY_G_NUM, HID_KEY_G},
    {CONFIG_KEY_H_NUM, HID_KEY_H},
    {CONFIG_KEY_I_NUM, HID_KEY_I},
    {CONFIG_KEY_J_NUM, HID_KEY_J},
    {CONFIG_KEY_K_NUM, HID_KEY_K},
    {CONFIG_KEY_L_NUM, HID_KEY_L},
    {CONFIG_KEY_M_NUM, HID_KEY_M},
    {CONFIG_KEY_N_NUM, HID_KEY_N},
    {CONFIG_KEY_O_NUM, HID_KEY_O},
    {CONFIG_KEY_P_NUM, HID_KEY_P},
    {CONFIG_KEY_Q_NUM, HID_KEY_Q},
    {CONFIG_KEY_R_NUM, HID_KEY_R},
    {CONFIG_KEY_S_NUM, HID_KEY_S},
    {CONFIG_KEY_T_NUM, HID_KEY_T},
    {CONFIG_KEY_U_NUM, HID_KEY_U},
    {CONFIG_KEY_V_NUM, HID_KEY_V},
    {CONFIG_KEY_W_NUM, HID_KEY_W},
    {CONFIG_KEY_X_NUM, HID_KEY_X},
    {CONFIG_KEY_Y_NUM, HID_KEY_Y},
    {CONFIG_KEY_Z_NUM, HID_KEY_Z},
    {CONFIG_KEY_SPACE_NUM, HID_KEY_SPACE},
    {CONFIG_KEY_ENTER_NUM, HID_KEY_ENTER},
    {CONFIG_KEY_ESCAPE_NUM, HID_KEY_ESCAPE},
    {CONFIG_KEY_CAPS_LOCK_NUM, HID_KEY_CAPS_LOCK},
    {CONFIG_KEY_TAB_NUM, HID_KEY_TAB},
    {CONFIG_KEY_SEMICOLON_NUM, HID_KEY_SEMICOLON},
    {CONFIG_KEY_APOSTROPHE_NUM, HID_KEY_APOSTROPHE},
    {CONFIG_KEY_GRAVE_NUM, HID_KEY_GRAVE},
    {CONFIG_KEY_COMMA_NUM, HID_KEY_COMMA},
    {CONFIG_KEY_PERIOD_NUM, HID_KEY_PERIOD},
    {CONFIG_KEY_SLASH_NUM, HID_KEY_SLASH},
    {CONFIG_KEY_BACKSLASH_NUM, HID_KEY_BACKSLASH},
    {CONFIG_KEY_BRACKET_LEFT_NUM, HID_KEY_BRACKET_LEFT},
    {CONFIG_KEY_BRACKET_RIGHT_NUM, HID_KEY_BRACKET_RIGHT},
    {CONFIG_KEY_ARROW_LEFT_NUM, HID_KEY_ARROW_LEFT},
    {CONFIG_KEY_ARROW_DOWN_NUM, HID_KEY_ARROW_DOWN},
    {CONFIG_KEY_ARROW_RIGHT_NUM, HID_KEY_ARROW_RIGHT},
    {CONFIG_KEY_ARROW_UP_NUM, HID_KEY_ARROW_UP},
    {CONFIG_KEY_1_NUM, HID_KEY_1},
    {CONFIG_KEY_2_NUM, HID_KEY_2},
    {CONFIG_KEY_3_NUM, HID_KEY_3},
    {CONFIG_KEY_4_NUM, HID_KEY_4},
    {CONFIG_KEY_5_NUM, HID_KEY_5},
    {CONFIG_KEY_6_NUM, HID_KEY_6},
    {CONFIG_KEY_7_NUM, HID_KEY_7},
    {CONFIG_KEY_8_NUM, HID_KEY_8},
    {CONFIG_KEY_9_NUM, HID_KEY_9},
    {CONFIG_KEY_0_NUM, HID_KEY_0},
    {CONFIG_KEY_MINUS_NUM, HID_KEY_MINUS},
    {CONFIG_KEY_EQUAL_NUM, HID_KEY_EQUAL},
    {CONFIG_KEY_F1_NUM, HID_KEY_F1},
    {CONFIG_KEY_F2_NUM, HID_KEY_F2},
    {CONFIG_KEY_F3_NUM, HID_KEY_F3},
    {CONFIG_KEY_F4_NUM, HID_KEY_F4},
    {CONFIG_KEY_F5_NUM, HID_KEY_F5},
    {CONFIG_KEY_F6_NUM, HID_KEY_F6},
    {CONFIG_KEY_F7_NUM, HID_KEY_F7},
    {CONFIG_KEY_F8_NUM, HID_KEY_F8},
    {CONFIG_KEY_F9_NUM, HID_KEY_F9},
    {CONFIG_KEY_F10_NUM, HID_KEY_F10},
    {CONFIG_KEY_F11_NUM, HID_KEY_F11},
    {CONFIG_KEY_F12_NUM, HID_KEY_F12},
    {CONFIG_KEY_BACKSPACE_NUM, HID_KEY_BACKSPACE},
    {CONFIG_KEY_DELETE_NUM, HID_KEY_DELETE},
    {CONFIG_KEY_INSERT_NUM, HID_KEY_INSERT},
};

const double_layer_key_t double_layer_keys[] = {
    {CONFIG_KEY_HOME_NUM, HID_KEY_HOME},
    {CONFIG_KEY_PAGE_DOWN_NUM, HID_KEY_PAGE_DOWN},
    {CONFIG_KEY_END_NUM, HID_KEY_END},
    {CONFIG_KEY_PAGE_UP_NUM, HID_KEY_PAGE_UP},
    {CONFIG_KEY_KEYPAD_1_NUM, HID_KEY_KEYPAD_1},
    {CONFIG_KEY_KEYPAD_2_NUM, HID_KEY_KEYPAD_2},
    {CONFIG_KEY_KEYPAD_3_NUM, HID_KEY_KEYPAD_3},
    {CONFIG_KEY_KEYPAD_4_NUM, HID_KEY_KEYPAD_4},
    {CONFIG_KEY_KEYPAD_5_NUM, HID_KEY_KEYPAD_5},
    {CONFIG_KEY_KEYPAD_6_NUM, HID_KEY_KEYPAD_6},
    {CONFIG_KEY_KEYPAD_7_NUM, HID_KEY_KEYPAD_7},
    {CONFIG_KEY_KEYPAD_8_NUM, HID_KEY_KEYPAD_8},
    {CONFIG_KEY_KEYPAD_9_NUM, HID_KEY_KEYPAD_9},
    {CONFIG_KEY_KEYPAD_0_NUM, HID_KEY_KEYPAD_0},
    {CONFIG_KEY_KEYPAD_SUBTRACT_NUM, HID_KEY_KEYPAD_SUBTRACT},
    {CONFIG_KEY_KEYPAD_ADD_NUM, HID_KEY_KEYPAD_ADD},
    {CONFIG_KEY_KEYPAD_DIVIDE_NUM, HID_KEY_KEYPAD_DIVIDE},
};

const modifier_mapping_t modifier_key_mappings[] = {
    {CONFIG_KEY_LEFT_CTRL_NUM, KEYBOARD_MODIFIER_LEFTCTRL},
    {CONFIG_KEY_LEFT_SHIFT_NUM, KEYBOARD_MODIFIER_LEFTSHIFT},
    {CONFIG_KEY_LEFT_ALT_NUM, KEYBOARD_MODIFIER_LEFTALT},
    {CONFIG_KEY_LEFT_GUI_NUM, KEYBOARD_MODIFIER_LEFTGUI},
    {CONFIG_KEY_RIGHT_CTRL_NUM, KEYBOARD_MODIFIER_RIGHTCTRL},
    {CONFIG_KEY_RIGHT_SHIFT_NUM, KEYBOARD_MODIFIER_RIGHTSHIFT},
    {CONFIG_KEY_RIGHT_ALT_NUM, KEYBOARD_MODIFIER_RIGHTALT},
    {CONFIG_KEY_RIGHT_GUI_NUM, KEYBOARD_MODIFIER_RIGHTGUI},
};

static bool handle_profile_consumer_action(uint16_t usage_code, TickType_t current_time)
{
    if ((current_time - last_volume_time) < pdMS_TO_TICKS(VOLUME_CONTROL_COOLDOWN_MS)) {
        return true;
    }

    send_consumer_control(usage_code);
    last_volume_time = current_time;
    return true;
}

static void handle_profile_led_action(uint8_t action_type, TickType_t current_time)
{
    if ((current_time - last_led_switch_time) < pdMS_TO_TICKS(LED_CONTROL_COOLDOWN_MS)) {
        return;
    }

    switch (action_type) {
    case YBK_ACTION_LED_TOGGLE:
        keyboard_profile_set_led_enabled(!led_state);
        ESP_LOGI(TAG, "LED state changed: %s", led_state ? "ON" : "OFF");
        break;
    case YBK_ACTION_LED_BRIGHTNESS_UP:
        keyboard_profile_adjust_brightness(10);
        ESP_LOGI(TAG, "LED brightness increased");
        break;
    case YBK_ACTION_LED_BRIGHTNESS_DOWN:
        keyboard_profile_adjust_brightness(-10);
        ESP_LOGI(TAG, "LED brightness decreased");
        break;
    case YBK_ACTION_LED_EFFECT_NEXT:
        keyboard_profile_next_led_mode();
        ESP_LOGI(TAG, "LED effect changed");
        break;
    default:
        return;
    }

    last_led_switch_time = current_time;
}

void process_key_press(int *pressed_pins, int num_pressed_pins)
{
    if (num_pressed_pins == 0) {
        tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);
        return;
    }

    uint8_t keycode[MAX_PRESSED_KEYS] = {0};
    int keycode_index = 0;
    hid_keyboard_modifier_bm_t modifier = 0;
    bool fn_pressed = false;
    TickType_t current_time = xTaskGetTickCount();

    for (int i = 0; i < num_pressed_pins; i++) {
        const keyboard_action_t *action =
            keyboard_profile_get_action(YBK_LAYER_BASE, (uint8_t)pressed_pins[i]);
        if (action->type == YBK_ACTION_LAYER_FN) {
            fn_pressed = true;
            break;
        }
    }

    for (int i = 0; i < num_pressed_pins; i++) {
        uint8_t current_pin = (uint8_t)pressed_pins[i];
        const keyboard_action_t *base_action = keyboard_profile_get_action(YBK_LAYER_BASE, current_pin);
        const keyboard_action_t *fn_action = keyboard_profile_get_action(YBK_LAYER_FN, current_pin);
        const keyboard_action_t *action = base_action;

        if (base_action->type == YBK_ACTION_LAYER_FN) {
            continue;
        }

        if (fn_pressed) {
            if (fn_action->type != YBK_ACTION_NONE) {
                action = fn_action;
            } else if (base_action->type != YBK_ACTION_MODIFIER) {
                continue;
            }
        }

        switch (action->type) {
        case YBK_ACTION_KEY:
            if (keycode_index < MAX_PRESSED_KEYS) {
                keycode[keycode_index++] = (uint8_t)action->code;
            }
            break;
        case YBK_ACTION_MODIFIER:
            modifier |= (hid_keyboard_modifier_bm_t)action->code;
            break;
        case YBK_ACTION_CONSUMER:
            handle_profile_consumer_action(action->code, current_time);
            break;
        case YBK_ACTION_LED_TOGGLE:
        case YBK_ACTION_LED_BRIGHTNESS_UP:
        case YBK_ACTION_LED_BRIGHTNESS_DOWN:
        case YBK_ACTION_LED_EFFECT_NEXT:
            handle_profile_led_action(action->type, current_time);
            break;
        default:
            break;
        }
    }

    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, modifier, keycode);
}
