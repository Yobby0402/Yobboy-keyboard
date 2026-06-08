/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "sdkconfig.h"
#include "keyboard_lighting_topology.h"
#include "pixel.h"
#include "esp_log.h"

extern bool led_state;

static const char *TAG = "pixel";

#define NEOPIXEL_MAX_KEYS 80
#define NEOPIXEL_MAX_RIPPLES 6

static const uint8_t _NeoPixelSineTable[256] = {
    128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 167, 170,
    173, 176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211,
    213, 215, 218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 238, 240,
    241, 243, 244, 245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254,
    254, 255, 255, 255, 255, 255, 255, 255, 254, 254, 254, 253, 253, 252, 251,
    250, 250, 249, 248, 246, 245, 244, 243, 241, 240, 238, 237, 235, 234, 232,
    230, 228, 226, 224, 222, 220, 218, 215, 213, 211, 208, 206, 203, 201, 198,
    196, 193, 190, 188, 185, 182, 179, 176, 173, 170, 167, 165, 162, 158, 155,
    152, 149, 146, 143, 140, 137, 134, 131, 128, 124, 121, 118, 115, 112, 109,
    106, 103, 100, 97, 93, 90, 88, 85, 82, 79, 76, 73, 70, 67, 65,
    62, 59, 57, 54, 52, 49, 47, 44, 42, 40, 37, 35, 33, 31, 29,
    27, 25, 23, 21, 20, 18, 17, 15, 14, 12, 11, 10, 9, 7, 6,
    5, 5, 4, 3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 2, 2, 3, 4, 5, 5, 6, 7, 9, 10, 11,
    12, 14, 15, 17, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35, 37,
    40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76,
    79, 82, 85, 88, 90, 93, 97, 100, 103, 106, 109, 112, 115, 118, 121,
    124
};

typedef struct {
    bool active;
    float x;
    float y;
    float z;
    uint16_t age;
} RippleEvent;

typedef struct {
    LampColor *pixels;
    LampColor *custom_pixels;
    uint8_t *fade_levels;
    uint32_t pixel_cnt;
    NeopixelEffect currentEffect;
    LampColor startingColor;
    led_strip_handle_t handle;
    uint8_t effectSpeed;
    bool last_pressed[NEOPIXEL_MAX_KEYS];
    RippleEvent ripples[NEOPIXEL_MAX_RIPPLES];
} NeopixelController;

static NeopixelController Controller = {0};

static void clear_runtime_state(void)
{
    if (Controller.fade_levels != NULL) {
        memset(Controller.fade_levels, 0, Controller.pixel_cnt * sizeof(*Controller.fade_levels));
    }
    memset(Controller.last_pressed, 0, sizeof(Controller.last_pressed));
    memset(Controller.ripples, 0, sizeof(Controller.ripples));
}

static void clear_pixels(LampColor *pixels, uint32_t count)
{
    if (pixels == NULL || count == 0) {
        return;
    }
    memset(pixels, 0, count * sizeof(*pixels));
}

static void fill_pixels(LampColor *pixels, uint32_t count, LampColor color)
{
    if (pixels == NULL) {
        return;
    }
    for (uint32_t i = 0; i < count; i++) {
        pixels[i] = color;
    }
}

static LampColor scale_color(LampColor color, uint8_t scale)
{
    LampColor scaled = {
        .red = (uint8_t)(((uint32_t)color.red * scale) / 255u),
        .green = (uint8_t)(((uint32_t)color.green * scale) / 255u),
        .blue = (uint8_t)(((uint32_t)color.blue * scale) / 255u),
        .intensity = 0,
    };
    return scaled;
}

static void copy_custom_pixels_to_output(void)
{
    if (Controller.pixels == NULL || Controller.custom_pixels == NULL) {
        return;
    }
    memcpy(Controller.pixels, Controller.custom_pixels, Controller.pixel_cnt * sizeof(*Controller.pixels));
}

static void blend_max_color(LampColor *dst, LampColor src)
{
    if (dst == NULL) {
        return;
    }
    if (src.red > dst->red) {
        dst->red = src.red;
    }
    if (src.green > dst->green) {
        dst->green = src.green;
    }
    if (src.blue > dst->blue) {
        dst->blue = src.blue;
    }
}

static uint32_t mapped_lamp_count(void)
{
    uint32_t topology_count = keyboard_lighting_topology_get_led_count();
    uint32_t limit = Controller.pixel_cnt < topology_count ? Controller.pixel_cnt : topology_count;
    return limit < YBK_LIGHTING_MAX_LAMPS ? limit : YBK_LIGHTING_MAX_LAMPS;
}

static void trigger_reactive_fade_for_key(uint8_t key_num)
{
    if (Controller.fade_levels == NULL) {
        return;
    }

    uint32_t lamp_count = mapped_lamp_count();
    for (uint32_t lamp = 0; lamp < lamp_count; lamp++) {
        if (keyboard_lighting_topology_key_for_led((uint16_t)lamp) == key_num) {
            Controller.fade_levels[lamp] = 255;
        }
    }
}

static bool find_key_center(uint8_t key_num, float *x, float *y, float *z)
{
    uint32_t lamp_count = mapped_lamp_count();
    float sum_x = 0.0f;
    float sum_y = 0.0f;
    float sum_z = 0.0f;
    uint32_t count = 0;

    for (uint32_t lamp = 0; lamp < lamp_count; lamp++) {
        const Position *position = keyboard_lighting_topology_position_for_led((uint16_t)lamp);
        if (keyboard_lighting_topology_key_for_led((uint16_t)lamp) != key_num || position == NULL) {
            continue;
        }
        sum_x += (float)position->x;
        sum_y += (float)position->y;
        sum_z += (float)position->z;
        count++;
    }

    if (count == 0) {
        return false;
    }

    *x = sum_x / (float)count;
    *y = sum_y / (float)count;
    *z = sum_z / (float)count;
    return true;
}

static void spawn_ripple_for_key(uint8_t key_num)
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    if (!find_key_center(key_num, &x, &y, &z)) {
        return;
    }

    RippleEvent *slot = NULL;
    for (uint8_t i = 0; i < NEOPIXEL_MAX_RIPPLES; i++) {
        if (!Controller.ripples[i].active) {
            slot = &Controller.ripples[i];
            break;
        }
    }
    if (slot == NULL) {
        slot = &Controller.ripples[0];
        for (uint8_t i = 1; i < NEOPIXEL_MAX_RIPPLES; i++) {
            if (Controller.ripples[i].age > slot->age) {
                slot = &Controller.ripples[i];
            }
        }
    }

    slot->active = true;
    slot->x = x;
    slot->y = y;
    slot->z = z;
    slot->age = 0;
}

void NeopixelInit(led_strip_handle_t handle, uint32_t pixel_cnt)
{
    Controller.pixels = (LampColor *)calloc(pixel_cnt, sizeof(LampColor));
    Controller.custom_pixels = (LampColor *)calloc(pixel_cnt, sizeof(LampColor));
    Controller.fade_levels = (uint8_t *)calloc(pixel_cnt, sizeof(uint8_t));
    if (Controller.pixels == NULL || Controller.custom_pixels == NULL || Controller.fade_levels == NULL) {
        free(Controller.pixels);
        free(Controller.custom_pixels);
        free(Controller.fade_levels);
        memset(&Controller, 0, sizeof(Controller));
        ESP_LOGE(TAG, "Failed to allocate pixel buffers");
        return;
    }

    Controller.pixel_cnt = pixel_cnt;
    Controller.handle = handle;
    Controller.currentEffect = HID;
    Controller.effectSpeed = 50;

    ESP_LOGI(TAG, "Neopixel initialized: %lu pixels", (unsigned long)pixel_cnt);
}

void NeopixelSetEffect(NeopixelEffect effect, LampColor effectColor, uint8_t speed)
{
    Controller.currentEffect = effect;
    Controller.startingColor = effectColor;
    Controller.effectSpeed = speed;
    clear_runtime_state();

    if (effect == STATIC_MAP) {
        copy_custom_pixels_to_output();
        return;
    }

    if (effect == REACTIVE_FADE || effect == RIPPLE) {
        clear_pixels(Controller.pixels, Controller.pixel_cnt);
        return;
    }

    fill_pixels(Controller.pixels, Controller.pixel_cnt, effectColor);

    ESP_LOGI(TAG, "Effect set to %d, color RGB(%u,%u,%u)",
             effect, effectColor.red, effectColor.green, effectColor.blue);
}

void NeopixelSetCustomColors(const LampColor *colors, uint32_t count)
{
    if (colors == NULL || Controller.custom_pixels == NULL) {
        return;
    }

    uint32_t limit = count < Controller.pixel_cnt ? count : Controller.pixel_cnt;
    for (uint32_t i = 0; i < limit; i++) {
        Controller.custom_pixels[i] = colors[i];
    }
    for (uint32_t i = limit; i < Controller.pixel_cnt; i++) {
        Controller.custom_pixels[i] = (LampColor){0};
    }

    if (Controller.currentEffect == STATIC_MAP) {
        copy_custom_pixels_to_output();
    }
}

void NeopixelNotifyPressedKeys(const int *pressed_keys, uint8_t count)
{
    bool current_pressed[NEOPIXEL_MAX_KEYS] = {0};

    for (uint8_t i = 0; i < count; i++) {
        int key_num = pressed_keys[i];
        if (key_num < 0 || key_num >= NEOPIXEL_MAX_KEYS) {
            continue;
        }
        current_pressed[key_num] = true;
    }

    if (Controller.currentEffect == REACTIVE_FADE || Controller.currentEffect == RIPPLE) {
        for (uint8_t key_num = 0; key_num < NEOPIXEL_MAX_KEYS; key_num++) {
            if (!current_pressed[key_num] || Controller.last_pressed[key_num]) {
                continue;
            }
            if (Controller.currentEffect == REACTIVE_FADE) {
                trigger_reactive_fade_for_key(key_num);
            } else {
                spawn_ripple_for_key(key_num);
            }
        }
    }

    memcpy(Controller.last_pressed, current_pressed, sizeof(Controller.last_pressed));
}

void NeopixelSetColor(uint16_t lampId, LampColor lampColor)
{
    if (lampId >= Controller.pixel_cnt) {
        return;
    }

    Controller.pixels[lampId] = lampColor;
}

void NeopixelSetColorRange(uint16_t lampIdStart, uint16_t lampIdEnd, LampColor lampColor)
{
    if (lampIdStart >= Controller.pixel_cnt) {
        return;
    }

    for (uint16_t i = lampIdStart; i <= lampIdEnd && i < Controller.pixel_cnt; i++) {
        NeopixelSetColor(i, lampColor);
    }
}

void NeopixelSendColors(void)
{
    static bool last_enabled_state = false;
    static bool fade_in_active = false;
    static uint32_t fade_step = 0;

    if (Controller.handle == NULL) {
        return;
    }

    const uint32_t fade_steps_config = CONFIG_LED_POWER_ON_FADE_STEPS;
    const uint32_t fade_steps = (fade_steps_config == 0) ? 1 : fade_steps_config;
    const bool fade_supported = (fade_steps > 1);

    if (!led_state) {
        if (last_enabled_state) {
            led_strip_clear(Controller.handle);
            led_strip_refresh(Controller.handle);
            last_enabled_state = false;
        }
        fade_in_active = false;
        fade_step = 0;
        return;
    }

    if (!last_enabled_state) {
        last_enabled_state = true;
        fade_step = 0;
        fade_in_active = fade_supported;
    }

    uint32_t current_step = fade_steps;
    bool apply_scaling = false;

    if (fade_in_active) {
        if (fade_step < fade_steps) {
            fade_step++;
        }

        current_step = (fade_step > fade_steps) ? fade_steps : fade_step;
        apply_scaling = (current_step < fade_steps);

        if (fade_step >= fade_steps) {
            fade_in_active = false;
        }
    }

    for (uint32_t i = 0; i < Controller.pixel_cnt; i++) {
        uint32_t red = Controller.pixels[i].red;
        uint32_t green = Controller.pixels[i].green;
        uint32_t blue = Controller.pixels[i].blue;

        if (apply_scaling) {
            red = (red * current_step) / fade_steps;
            green = (green * current_step) / fade_steps;
            blue = (blue * current_step) / fade_steps;
        }

        led_strip_set_pixel(Controller.handle, i,
                            (uint8_t)red,
                            (uint8_t)green,
                            (uint8_t)blue);
    }
    led_strip_refresh(Controller.handle);
}

static void NeopixelSineWaveStep(void)
{
    static uint16_t t = 0;
    uint8_t step = (uint8_t)(1u + (Controller.effectSpeed / 20u));

    for (uint32_t i = 0; i < Controller.pixel_cnt; i++) {
        uint8_t phase = (uint8_t)t;
        Controller.pixels[i].red = (uint8_t)(((uint32_t)Controller.startingColor.red * _NeoPixelSineTable[phase]) >> 8);
        Controller.pixels[i].green = (uint8_t)(((uint32_t)Controller.startingColor.green * _NeoPixelSineTable[phase]) >> 8);
        Controller.pixels[i].blue = (uint8_t)(((uint32_t)Controller.startingColor.blue * _NeoPixelSineTable[phase]) >> 8);
    }

    t = (uint16_t)(t + step);
}

static LampColor wheel(uint8_t pos)
{
    pos = 255 - pos;
    if (pos < 85) {
        return (LampColor){255 - pos * 3, 0, pos * 3, 0};
    }
    if (pos < 170) {
        pos -= 85;
        return (LampColor){0, pos * 3, 255 - pos * 3, 0};
    }
    pos -= 170;
    return (LampColor){pos * 3, 255 - pos * 3, 0, 0};
}

static void NeopixelRainbowStep(void)
{
    static uint16_t t = 0;

    if (Controller.pixel_cnt == 0) {
        return;
    }

    for (uint32_t i = 0; i < Controller.pixel_cnt; i++) {
        uint8_t pos = (uint8_t)(((i * 256u) / Controller.pixel_cnt + t) & 0xffu);
        Controller.pixels[i] = wheel(pos);
    }

    t = (uint16_t)(t + (Controller.effectSpeed > 0 ? Controller.effectSpeed : 1));
}

static void NeopixelWaveStep(void)
{
    static uint16_t t = 0;
    uint8_t step = (uint8_t)(1u + (Controller.effectSpeed / 5u));

    for (uint32_t i = 0; i < Controller.pixel_cnt; i++) {
        uint8_t wave = _NeoPixelSineTable[(uint8_t)(t + i * 8u)];
        Controller.pixels[i].red = (uint8_t)(((uint32_t)Controller.startingColor.red * wave) >> 8);
        Controller.pixels[i].green = (uint8_t)(((uint32_t)Controller.startingColor.green * wave) >> 8);
        Controller.pixels[i].blue = (uint8_t)(((uint32_t)Controller.startingColor.blue * wave) >> 8);
    }

    t = (uint16_t)(t + step);
}

static void NeopixelReactiveFadeStep(void)
{
    uint8_t decay = (uint8_t)(2u + (Controller.effectSpeed / 8u));

    for (uint32_t i = 0; i < Controller.pixel_cnt; i++) {
        uint8_t level = Controller.fade_levels[i];
        uint8_t eased = (uint8_t)(((uint16_t)level * (uint16_t)level) / 255u);
        Controller.pixels[i] = scale_color(Controller.startingColor, eased);

        if (level <= decay) {
            Controller.fade_levels[i] = 0;
        } else {
            Controller.fade_levels[i] = (uint8_t)(level - decay);
        }
    }
}

static void NeopixelRippleStep(void)
{
    const float velocity = 3000.0f + ((float)Controller.effectSpeed * 240.0f);
    const float width = 12000.0f + ((float)Controller.effectSpeed * 90.0f);
    const uint16_t max_age = (uint16_t)(28u + ((100u - Controller.effectSpeed) * 3u / 5u));

    clear_pixels(Controller.pixels, Controller.pixel_cnt);

    for (uint8_t ripple_index = 0; ripple_index < NEOPIXEL_MAX_RIPPLES; ripple_index++) {
        RippleEvent *ripple = &Controller.ripples[ripple_index];
        if (!ripple->active) {
            continue;
        }

        if (ripple->age > max_age) {
            ripple->active = false;
            continue;
        }

        float radius = (float)ripple->age * velocity;
        float life = 1.0f - ((float)ripple->age / (float)(max_age + 1u));

        for (uint32_t lamp = 0; lamp < Controller.pixel_cnt; lamp++) {
            const Position *position = keyboard_lighting_topology_position_for_led((uint16_t)lamp);
            if (position == NULL) {
                continue;
            }
            float dx = (float)position->x - ripple->x;
            float dy = (float)position->y - ripple->y;
            float dz = (float)position->z - ripple->z;
            float distance = sqrtf(dx * dx + dy * dy + dz * dz);
            float delta = fabsf(distance - radius);

            if (delta > width) {
                continue;
            }

            float band = 1.0f - (delta / width);
            float amplitude = band * life;
            if (amplitude <= 0.0f) {
                continue;
            }

            uint8_t scale = (uint8_t)(amplitude * 255.0f);
            blend_max_color(&Controller.pixels[lamp], scale_color(Controller.startingColor, scale));
        }

        ripple->age++;
    }
}

void NeopixelUpdateEffect(void)
{
    switch (Controller.currentEffect) {
    case BREATH:
        NeopixelSineWaveStep();
        break;
    case RAINBOW:
        NeopixelRainbowStep();
        break;
    case WAVE:
        NeopixelWaveStep();
        break;
    case REACTIVE_FADE:
        NeopixelReactiveFadeStep();
        break;
    case RIPPLE:
        NeopixelRippleStep();
        break;
    case STATIC_MAP:
    case HID:
    case SOLID:
    default:
        break;
    }

    NeopixelSendColors();
}
