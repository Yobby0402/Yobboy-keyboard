#include "key_bind.h"

/**
 * @brief Bind basic keys' numbers and their codes,these keys have no further function.
 */
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

};

/**
 * @brief Bind modifier keys' numbers and their codes, such as ctrl alt and shift.
 */
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

void process_key_press(int* pressed_pins, int num_pressed_pins) {
    if (num_pressed_pins == 0) {
        tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);
        return;
    }

    // 声明并初始化 keycode 数组，用于存储键码
    uint8_t keycode[6] = {0};
    int keycode_index = 0; // 用于记录 keycode 数组的当前索引位置

    // 计算修饰符
    hid_keyboard_modifier_bm_t modifier = 0;
    for (int i = 0; i < num_pressed_pins; i++) {
        for (int j = 0; j < sizeof(modifier_key_mappings) / sizeof(modifier_mapping_t); j++) {
            if (pressed_pins[i] == modifier_key_mappings[j].key_num) {
                modifier |= modifier_key_mappings[j].hid_modifier;
            }
        }
    }

    // 检查是否有 Fn 键被按下
    int fn_pressed = 0;
    for (int i = 0; i < num_pressed_pins; i++) {
        if (pressed_pins[i] == CONFIG_KEY_FN_NUM) {
            fn_pressed = 1;
            break;
        }
    }
    
    if(fn_pressed && num_pressed_pins==2) {
        if (pressed_pins[0]==CONFIG_KEY_LED_SWITCH_NUM || pressed_pins[1]==CONFIG_KEY_LED_SWITCH_NUM){
            led_state = !led_state;
            vTaskDelay(pdMS_TO_TICKS(CONFIG_LED_SWITCH_SLEEP));
        }
        else if (pressed_pins[0]==CONFIG_KEY_LED_ADD_NUM || pressed_pins[1]==CONFIG_KEY_LED_ADD_NUM){
            led_brightness += 10;
            if (led_brightness > 100) 
            {
                led_brightness = 100;
            }
            vTaskDelay(pdMS_TO_TICKS(CONFIG_LED_SWITCH_SLEEP));
        }
        else if (pressed_pins[0]==CONFIG_KEY_LED_SUB_NUM || pressed_pins[1]==CONFIG_KEY_LED_SUB_NUM){
            led_brightness -= 10;
            if (led_brightness < 0) 
            {
                led_brightness = 0;
            }
            vTaskDelay(pdMS_TO_TICKS(CONFIG_LED_SWITCH_SLEEP));
        }
        else if (pressed_pins[0]==CONFIG_KEY_LED_EFFECT_NUM || pressed_pins[1]==CONFIG_KEY_LED_EFFECT_NUM){
            led_effects = (led_effects + 1) % num_effects;
            vTaskDelay(pdMS_TO_TICKS(CONFIG_LED_SWITCH_SLEEP));
        }
        
    }

    // 遍历按下的按键，查找多层功能键
    for (int i = 0; i < num_pressed_pins; i++) {
        if (fn_pressed) {
            for (int j = 0; j < sizeof(double_layer_keys) / sizeof(double_layer_key_t); j++) {
                if (pressed_pins[i] == double_layer_keys[j].key_number) {
                    // 将多层功能键的键码存入 keycode 数组中
                    keycode[keycode_index++] = double_layer_keys[j].secondary_key_value;
                    // 如果 keycode 数组已满，退出循环
                    if (keycode_index >= 6) {
                        break;
                    }
                }
            }
        } else {
            // 如果 Fn 未被按下，则直接将基本键码存入 keycode 数组中
            for (int j = 0; j < sizeof(basic_key_mappings) / sizeof(basic_key_mapping_t); j++) {
                if (pressed_pins[i] == basic_key_mappings[j].config_key_num) {
                    keycode[keycode_index++] = basic_key_mappings[j].tusb_hid_key_num;
                    // 如果 keycode 数组已满，退出循环
                    if (keycode_index >= 6) {
                        break;
                    }
                }
            }
        }
        // 如果 keycode 数组已满，退出循环
        if (keycode_index >= 6) {
            break;
        }
    }

    // 发送键盘报文
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, modifier, keycode);
}
