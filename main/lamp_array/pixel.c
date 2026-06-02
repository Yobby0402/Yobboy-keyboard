/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * 适配：Yobboy 键盘（79 键）
 */

#include "sdkconfig.h"
#include "pixel.h"
#include "lamp_array_matrix.h"
#include "esp_log.h"

extern bool led_state;

static const char *TAG = "pixel";

// 正弦波查找表（用于呼吸灯效）
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
    106, 103, 100, 97,  93,  90,  88,  85,  82,  79,  76,  73,  70,  67,  65,
    62,  59,  57,  54,  52,  49,  47,  44,  42,  40,  37,  35,  33,  31,  29,
    27,  25,  23,  21,  20,  18,  17,  15,  14,  12,  11,  10,  9,   7,   6,
    5,   5,   4,   3,   2,   2,   1,   1,   1,   0,   0,   0,   0,   0,   0,
    0,   1,   1,   1,   2,   2,   3,   4,   5,   5,   6,   7,   9,   10,  11,
    12,  14,  15,  17,  18,  20,  21,  23,  25,  27,  29,  31,  33,  35,  37,
    40,  42,  44,  47,  49,  52,  54,  57,  59,  62,  65,  67,  70,  73,  76,
    79,  82,  85,  88,  90,  93,  97,  100, 103, 106, 109, 112, 115, 118, 121,
    124
};

typedef struct {
    LampColor *pixels;
    uint32_t pixel_cnt;
    NeopixelEffect currentEffect;
    LampColor startingColor;
    led_strip_handle_t handle;
} NeopixelController;

static NeopixelController Controller = {0};

void NeopixelInit(led_strip_handle_t handle, uint32_t pixel_cnt)
{
    Controller.pixels = (LampColor *)calloc(pixel_cnt, sizeof(LampColor));
    if (Controller.pixels == NULL) {
        ESP_LOGE(TAG, "Failed to allocate pixel buffer");
        return;
    }
    
    Controller.pixel_cnt = pixel_cnt;
    Controller.handle = handle;
    Controller.currentEffect = HID;  // 默认 Windows 控制模式
    
    ESP_LOGI(TAG, "Neopixel initialized: %lu pixels", (unsigned long)pixel_cnt);
}

void NeopixelSetEffect(NeopixelEffect effect, LampColor effectColor)
{
    Controller.currentEffect = effect;
    Controller.startingColor = effectColor;

    for (uint32_t i = 0; i < Controller.pixel_cnt; i++) {
        Controller.pixels[i] = effectColor;
    }
    
    ESP_LOGI(TAG, "Effect set to %d, color RGB(%u,%u,%u)", 
             effect, effectColor.red, effectColor.green, effectColor.blue);
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

    for (int i = lampIdStart; i <= lampIdEnd && i < Controller.pixel_cnt; i++) {
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

void NeopixelSineWaveStep(void)
{
    static uint8_t t = 0;

    for (uint32_t i = 0; i < Controller.pixel_cnt; i++) {
        uint16_t outputColor;

        outputColor = (Controller.startingColor.red * _NeoPixelSineTable[t]) >> 8;
        Controller.pixels[i].red = outputColor;

        outputColor = (Controller.startingColor.green * _NeoPixelSineTable[t]) >> 8;
        Controller.pixels[i].green = outputColor;

        outputColor = (Controller.startingColor.blue * _NeoPixelSineTable[t]) >> 8;
        Controller.pixels[i].blue = outputColor;
    }

    t++;
}

void NeopixelUpdateEffect(void)
{
    if (Controller.currentEffect == BLINK) {
        // 呼吸灯模式
        NeopixelSineWaveStep();
    }
    // HID 模式：颜色由 Windows 控制，这里只需要刷新
    // SOLID 模式：纯色，不需要更新

    NeopixelSendColors();
}

