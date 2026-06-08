#include "keyboard_input.h"

#include <string.h>
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "keyboard_profile.h"

typedef enum {
    SOCD_SIDE_NONE = -1,
    SOCD_SIDE_FIRST = 0,
    SOCD_SIDE_SECOND = 1,
} socd_side_t;

typedef struct {
    bool first_down;
    bool second_down;
    socd_side_t active_side;
    socd_side_t pending_side;
    TickType_t pending_until;
} socd_pair_state_t;

typedef struct {
    bool first_down;
    bool second_down;
    socd_side_t synthetic_side;
    TickType_t synthetic_from;
    TickType_t synthetic_until;
} reverse_tap_pair_state_t;

static socd_pair_state_t s_socd_horizontal = {0};
static socd_pair_state_t s_socd_vertical = {0};
static reverse_tap_pair_state_t s_reverse_tap_horizontal = {0};
static reverse_tap_pair_state_t s_reverse_tap_vertical = {0};

static bool keycode_in_nkro_range(uint8_t keycode)
{
    return keycode <= KEYBOARD_INPUT_NKRO_USAGE_MAX;
}

static void nkro_set_key(keyboard_input_report_t *report, uint8_t keycode, bool pressed)
{
    if (report == NULL || !keycode_in_nkro_range(keycode)) {
        return;
    }

    uint8_t byte_index = keycode / 8;
    uint8_t bit_mask = (uint8_t)(1u << (keycode % 8));
    if (pressed) {
        report->nkro_keys[byte_index] |= bit_mask;
    } else {
        report->nkro_keys[byte_index] &= (uint8_t)~bit_mask;
    }
}

static bool resolve_fn_pressed(const int *pressed_pins, int num_pressed_pins)
{
    for (int i = 0; i < num_pressed_pins; i++) {
        const keyboard_action_t *action =
            keyboard_profile_get_action(YBK_LAYER_BASE, (uint8_t)pressed_pins[i]);
        if (action->type == YBK_ACTION_LAYER_FN) {
            return true;
        }
    }
    return false;
}

static const keyboard_action_t *resolve_key_action(uint8_t key_num, bool fn_pressed)
{
    const keyboard_action_t *base_action = keyboard_profile_get_action(YBK_LAYER_BASE, key_num);
    const keyboard_action_t *fn_action = keyboard_profile_get_action(YBK_LAYER_FN, key_num);

    if (base_action->type == YBK_ACTION_LAYER_FN) {
        return NULL;
    }

    if (fn_pressed) {
        if (fn_action->type != YBK_ACTION_NONE) {
            return fn_action;
        }
        if (base_action->type != YBK_ACTION_MODIFIER) {
            return NULL;
        }
    }

    return base_action;
}

static bool report_has_keycode(const keyboard_input_report_t *report, uint8_t keycode)
{
    if (report == NULL || keycode == 0) {
        return false;
    }

    if (keycode_in_nkro_range(keycode)) {
        uint8_t byte_index = keycode / 8;
        uint8_t bit_mask = (uint8_t)(1u << (keycode % 8));
        return (report->nkro_keys[byte_index] & bit_mask) != 0;
    }

    for (uint8_t i = 0; i < report->keycode_count; i++) {
        if (report->keycodes[i] == keycode) {
            return true;
        }
    }
    return false;
}

static void report_remove_keycodes(keyboard_input_report_t *report, uint8_t first, uint8_t second)
{
    if (report == NULL) {
        return;
    }

    uint8_t write_index = 0;
    for (uint8_t read_index = 0; read_index < report->keycode_count; read_index++) {
        uint8_t keycode = report->keycodes[read_index];
        if (keycode == first || keycode == second) {
            continue;
        }
        report->keycodes[write_index++] = keycode;
    }

    nkro_set_key(report, first, false);
    nkro_set_key(report, second, false);

    uint8_t new_count = write_index;
    while (write_index < KEYBOARD_INPUT_MAX_KEYS) {
        report->keycodes[write_index++] = 0;
    }

    report->keycode_count = new_count;
}

static void report_append_keycode(keyboard_input_report_t *report, uint8_t keycode)
{
    if (report == NULL || keycode == 0) {
        return;
    }

    if (report_has_keycode(report, keycode)) {
        return;
    }

    if (report->keycode_count >= KEYBOARD_INPUT_MAX_KEYS) {
        nkro_set_key(report, keycode, true);
        return;
    }

    report->keycodes[report->keycode_count++] = keycode;
    nkro_set_key(report, keycode, true);
}

static bool tick_has_elapsed(TickType_t now, TickType_t target)
{
    return (int32_t)(now - target) >= 0;
}

static TickType_t randomizable_duration_ticks(uint8_t duration_ms, bool randomize)
{
    uint8_t bounded_duration = duration_ms;

    if (bounded_duration == 0) {
        return 0;
    }

    uint32_t effective_duration_ms = bounded_duration;
    if (randomize && bounded_duration > 1) {
        effective_duration_ms = (esp_random() % bounded_duration) + 1u;
    }

    return pdMS_TO_TICKS(effective_duration_ms);
}

static void socd_track_passthrough(socd_pair_state_t *state, bool first_down, bool second_down)
{
    if (state == NULL) {
        return;
    }

    state->first_down = first_down;
    state->second_down = second_down;
    state->pending_side = SOCD_SIDE_NONE;

    if (first_down && !second_down) {
        state->active_side = SOCD_SIDE_FIRST;
    } else if (!first_down && second_down) {
        state->active_side = SOCD_SIDE_SECOND;
    } else if (!first_down && !second_down) {
        state->active_side = SOCD_SIDE_NONE;
    }
}

static socd_side_t socd_select_output(socd_pair_state_t *state, bool first_down, bool second_down,
                                      TickType_t now, uint8_t delay_ms, bool randomize)
{
    bool first_new = first_down && !state->first_down;
    bool second_new = second_down && !state->second_down;

    if (!first_down && !second_down) {
        state->active_side = SOCD_SIDE_NONE;
        state->pending_side = SOCD_SIDE_NONE;
        return SOCD_SIDE_NONE;
    }

    if (!first_down || !second_down) {
        state->pending_side = SOCD_SIDE_NONE;
        state->active_side = first_down ? SOCD_SIDE_FIRST : SOCD_SIDE_SECOND;
        return state->active_side;
    }

    if (first_new != second_new) {
        socd_side_t target_side = first_new ? SOCD_SIDE_FIRST : SOCD_SIDE_SECOND;
        TickType_t delay_ticks = randomizable_duration_ticks(delay_ms, randomize);
        if (delay_ticks == 0) {
            state->active_side = target_side;
            state->pending_side = SOCD_SIDE_NONE;
            return state->active_side;
        }

        state->active_side = SOCD_SIDE_NONE;
        state->pending_side = target_side;
        state->pending_until = now + delay_ticks;
        return SOCD_SIDE_NONE;
    }

    if (state->pending_side != SOCD_SIDE_NONE) {
        if (!tick_has_elapsed(now, state->pending_until)) {
            state->active_side = SOCD_SIDE_NONE;
            return SOCD_SIDE_NONE;
        }
        state->active_side = state->pending_side;
        state->pending_side = SOCD_SIDE_NONE;
        return state->active_side;
    }

    if (state->active_side == SOCD_SIDE_NONE) {
        if (first_new) {
            state->active_side = SOCD_SIDE_FIRST;
        } else if (second_new) {
            state->active_side = SOCD_SIDE_SECOND;
        } else if (state->first_down && !state->second_down) {
            state->active_side = SOCD_SIDE_FIRST;
        } else if (!state->first_down && state->second_down) {
            state->active_side = SOCD_SIDE_SECOND;
        } else {
            state->active_side = SOCD_SIDE_SECOND;
        }
    }

    return state->active_side;
}

static void apply_socd_pair(keyboard_input_report_t *report, socd_pair_state_t *state,
                            uint8_t first_code, uint8_t second_code,
                            TickType_t now, uint8_t delay_ms, bool randomize)
{
    bool first_down = report_has_keycode(report, first_code);
    bool second_down = report_has_keycode(report, second_code);
    socd_side_t output_side = socd_select_output(state, first_down, second_down, now, delay_ms, randomize);

    state->first_down = first_down;
    state->second_down = second_down;

    report_remove_keycodes(report, first_code, second_code);
    if (output_side == SOCD_SIDE_FIRST) {
        report_append_keycode(report, first_code);
    } else if (output_side == SOCD_SIDE_SECOND) {
        report_append_keycode(report, second_code);
    }
}

static void reverse_tap_track_passthrough(reverse_tap_pair_state_t *state, bool first_down, bool second_down)
{
    if (state == NULL) {
        return;
    }

    state->first_down = first_down;
    state->second_down = second_down;
    state->synthetic_side = SOCD_SIDE_NONE;
    state->synthetic_from = 0;
    state->synthetic_until = 0;
}

static void apply_reverse_tap_pair(keyboard_input_report_t *report, reverse_tap_pair_state_t *state,
                                   bool first_physical_down, bool second_physical_down,
                                   uint8_t first_code, uint8_t second_code,
                                   TickType_t now, uint8_t delay_ms, uint8_t duration_ms, bool randomize)
{
    bool first_released = state->first_down && !first_physical_down;
    bool second_released = state->second_down && !second_physical_down;
    bool both_released = first_released && second_released;

    if (first_physical_down || second_physical_down) {
        state->synthetic_side = SOCD_SIDE_NONE;
        state->synthetic_from = 0;
        state->synthetic_until = 0;
    } else if (!both_released) {
        if (first_released) {
            TickType_t delay_ticks = randomizable_duration_ticks(delay_ms, randomize);
            TickType_t duration_ticks = randomizable_duration_ticks(duration_ms, randomize);
            state->synthetic_side = duration_ticks == 0 ? SOCD_SIDE_NONE : SOCD_SIDE_SECOND;
            state->synthetic_from = now + delay_ticks;
            state->synthetic_until = state->synthetic_from + duration_ticks;
        } else if (second_released) {
            TickType_t delay_ticks = randomizable_duration_ticks(delay_ms, randomize);
            TickType_t duration_ticks = randomizable_duration_ticks(duration_ms, randomize);
            state->synthetic_side = duration_ticks == 0 ? SOCD_SIDE_NONE : SOCD_SIDE_FIRST;
            state->synthetic_from = now + delay_ticks;
            state->synthetic_until = state->synthetic_from + duration_ticks;
        }
    }

    if (!first_physical_down && !second_physical_down && state->synthetic_side != SOCD_SIDE_NONE) {
        if (!tick_has_elapsed(now, state->synthetic_from)) {
            /* wait for reverse-tap delay */
        } else if (!tick_has_elapsed(now, state->synthetic_until)) {
            report_append_keycode(report,
                                  state->synthetic_side == SOCD_SIDE_FIRST ? first_code : second_code);
        } else {
            state->synthetic_side = SOCD_SIDE_NONE;
            state->synthetic_from = 0;
            state->synthetic_until = 0;
        }
    }

    state->first_down = first_physical_down;
    state->second_down = second_physical_down;
}

static void keyboard_input_apply_wasd_assists(keyboard_input_report_t *report)
{
    bool enabled = keyboard_profile_socd_enabled();
    bool randomize = keyboard_profile_socd_randomize();
    uint8_t delay_ms = keyboard_profile_socd_delay_ms();
    bool reverse_tap_enabled = keyboard_profile_reverse_tap_enabled();
    bool reverse_tap_randomize = keyboard_profile_reverse_tap_randomize();
    uint8_t reverse_tap_delay_ms = keyboard_profile_reverse_tap_delay_ms();
    uint8_t reverse_tap_duration_ms = keyboard_profile_reverse_tap_duration_ms();
    TickType_t now = xTaskGetTickCount();
    bool a_down = report_has_keycode(report, HID_KEY_A);
    bool d_down = report_has_keycode(report, HID_KEY_D);
    bool w_down = report_has_keycode(report, HID_KEY_W);
    bool s_down = report_has_keycode(report, HID_KEY_S);

    if (!enabled) {
        socd_track_passthrough(&s_socd_horizontal, a_down, d_down);
        socd_track_passthrough(&s_socd_vertical, w_down, s_down);
    } else {
        apply_socd_pair(report, &s_socd_horizontal, HID_KEY_A, HID_KEY_D, now, delay_ms, randomize);
        apply_socd_pair(report, &s_socd_vertical, HID_KEY_W, HID_KEY_S, now, delay_ms, randomize);
    }

    if (!reverse_tap_enabled) {
        reverse_tap_track_passthrough(&s_reverse_tap_horizontal, a_down, d_down);
        reverse_tap_track_passthrough(&s_reverse_tap_vertical, w_down, s_down);
        return;
    }

    apply_reverse_tap_pair(report, &s_reverse_tap_horizontal, a_down, d_down,
                           HID_KEY_A, HID_KEY_D, now,
                           reverse_tap_delay_ms, reverse_tap_duration_ms, reverse_tap_randomize);
    apply_reverse_tap_pair(report, &s_reverse_tap_vertical, w_down, s_down,
                           HID_KEY_W, HID_KEY_S, now,
                           reverse_tap_delay_ms, reverse_tap_duration_ms, reverse_tap_randomize);
}

void keyboard_input_build_report(const int *pressed_pins, int num_pressed_pins,
                                 keyboard_input_report_t *report)
{
    memset(report, 0, sizeof(*report));
    if (pressed_pins == NULL || num_pressed_pins <= 0) {
        keyboard_input_apply_wasd_assists(report);
        return;
    }

    report->fn_pressed = resolve_fn_pressed(pressed_pins, num_pressed_pins);

    for (int i = 0; i < num_pressed_pins; i++) {
        const keyboard_action_t *action =
            resolve_key_action((uint8_t)pressed_pins[i], report->fn_pressed);
        if (action == NULL) {
            continue;
        }

        switch (action->type) {
        case YBK_ACTION_KEY:
            report_append_keycode(report, (uint8_t)action->code);
            break;
        case YBK_ACTION_MODIFIER:
            report->modifier |= (hid_keyboard_modifier_bm_t)action->code;
            break;
        case YBK_ACTION_CONSUMER:
            if (report->consumer_count < KEYBOARD_INPUT_MAX_CONSUMERS) {
                report->consumer_usages[report->consumer_count++] = action->code;
            }
            break;
        case YBK_ACTION_LED_TOGGLE:
        case YBK_ACTION_LED_BRIGHTNESS_UP:
        case YBK_ACTION_LED_BRIGHTNESS_DOWN:
        case YBK_ACTION_LED_EFFECT_NEXT:
            if (report->led_action_count < KEYBOARD_INPUT_MAX_LED_ACTIONS) {
                report->led_actions[report->led_action_count++] = action->type;
            }
            break;
        case YBK_ACTION_POWER_MODE_NEXT:
            if (report->power_action_count < KEYBOARD_INPUT_MAX_POWER_ACTIONS) {
                report->power_actions[report->power_action_count++] = action->type;
            }
            break;
        default:
            break;
        }
    }

    keyboard_input_apply_wasd_assists(report);
}
