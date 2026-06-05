#include "key_bind.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "class/hid/hid_device.h"
#include "keyboard_input.h"
#include "keyboard_power_policy.h"
#include "keyboard_profile.h"
#include "keyboard_transport.h"
#include "esp_log.h"

static const char *TAG = "key_bind";

static TickType_t last_led_switch_time = 0;
static TickType_t last_volume_time = 0;
static TickType_t last_remote_wakeup_time = 0;
static TickType_t last_power_mode_switch_time = 0;

#define LED_CONTROL_COOLDOWN_MS CONFIG_LED_SWITCH_SLEEP
#define VOLUME_CONTROL_COOLDOWN_MS 200
#define REMOTE_WAKEUP_COOLDOWN_MS 250
#define POWER_MODE_SWITCH_COOLDOWN_MS 300

static void send_consumer_control(uint16_t usage_code)
{
    keyboard_transport_send_consumer_report(usage_code);
    vTaskDelay(pdMS_TO_TICKS(50));
    keyboard_transport_send_consumer_report(0);
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
        ESP_LOGI(TAG, "Lighting preset changed");
        break;
    default:
        return;
    }

    last_led_switch_time = current_time;
}

static void try_remote_wakeup(TickType_t current_time)
{
    if ((current_time - last_remote_wakeup_time) < pdMS_TO_TICKS(REMOTE_WAKEUP_COOLDOWN_MS)) {
        return;
    }

    if (keyboard_transport_request_remote_wakeup()) {
        ESP_LOGI(TAG, "USB remote wakeup requested");
        last_remote_wakeup_time = current_time;
    }
}

static void handle_power_action(uint8_t action_type, TickType_t current_time)
{
    if ((current_time - last_power_mode_switch_time) < pdMS_TO_TICKS(POWER_MODE_SWITCH_COOLDOWN_MS)) {
        return;
    }

    if (action_type != YBK_ACTION_POWER_MODE_NEXT) {
        return;
    }

    if (!keyboard_power_policy_cycle_mode()) {
        ESP_LOGI(TAG, "Power mode cycle ignored (disabled by profile)");
        return;
    }

    last_power_mode_switch_time = current_time;
}

void process_key_press(int *pressed_pins, int num_pressed_pins)
{
    keyboard_profile_note_pressed_keys(pressed_pins, num_pressed_pins);

    if (num_pressed_pins == 0) {
        keyboard_transport_send_keyboard_report(0, NULL);
        return;
    }

    TickType_t current_time = xTaskGetTickCount();
    keyboard_input_report_t report = {0};

    try_remote_wakeup(current_time);
    keyboard_input_build_report(pressed_pins, num_pressed_pins, &report);

    for (uint8_t i = 0; i < report.consumer_count; i++) {
        handle_profile_consumer_action(report.consumer_usages[i], current_time);
    }
    for (uint8_t i = 0; i < report.led_action_count; i++) {
        handle_profile_led_action(report.led_actions[i], current_time);
    }
    for (uint8_t i = 0; i < report.power_action_count; i++) {
        handle_power_action(report.power_actions[i], current_time);
    }

    keyboard_transport_send_keyboard_report(report.modifier, report.keycodes);
}
