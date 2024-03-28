#include "led.h"
bool led_state = false;

led_strip_handle_t configure_led(void)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_BLINK_GPIO,   // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_NUMBERS,        // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812,            // LED strip model
        .flags.invert_out = false,                // whether to invert the output signal
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        .rmt_channel = 0,
#else
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
        .flags.with_dma = false,               // DMA feature is available on ESP target like ESP32-S3
#endif
    };

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    return led_strip;
}

void breathled(void)
{
    led_strip_handle_t led_strip = configure_led();
    float hue = 0.0f;
    float delta_hue = LED_RAINBOW_HUE_INCREMENT;
    float brightness = 0.0f;
    float delta_brightness = 0.05f; // Adjust this value to control the speed of the breathing effect
    while (1){
        if (led_state) {
            // Calculate current hue
            float current_hue = hue;

            // Calculate RGB values based on current hue and brightness
            uint8_t r, g, b;
            hsv_to_rgb(current_hue, 1.0f, brightness, &r, &g, &b);

            // Set RGB values for all LEDs
            for (int i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, r, g, b));
            }

            ESP_ERROR_CHECK(led_strip_refresh(led_strip));

            // Update brightness
            brightness += delta_brightness;
            if (brightness >= 1.0f || brightness <= 0.0f) {
                delta_brightness = -delta_brightness;
            }

            // Update hue
            hue += delta_hue;
            if (hue >= 1.0f) {
                hue -= 1.0f;
            }

            vTaskDelay(pdMS_TO_TICKS(LED_BREATHING_SPEED));
        } else {
            // Turn off all LEDs
            ESP_ERROR_CHECK(led_strip_clear(led_strip));
            vTaskDelay(pdMS_TO_TICKS(100)); // Wait for 100ms
        }
    }
}


// Function to convert HSV to RGB
void hsv_to_rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b) {
    int i;
    float f, p, q, t;

    if (s == 0) {
        // Achromatic (grey)
        *r = *g = *b = (uint8_t)(v * 255);
        return;
    }

    h *= 6; // Sector 0 to 5
    i = (int)floor(h);
    f = h - i; // Fractional part of h
    p = v * (1 - s);
    q = v * (1 - s * f);
    t = v * (1 - s * (1 - f));

    switch (i) {
        case 0: *r = (uint8_t)(v * 255); *g = (uint8_t)(t * 255); *b = (uint8_t)(p * 255); break;
        case 1: *r = (uint8_t)(q * 255); *g = (uint8_t)(v * 255); *b = (uint8_t)(p * 255); break;
        case 2: *r = (uint8_t)(p * 255); *g = (uint8_t)(v * 255); *b = (uint8_t)(t * 255); break;
        case 3: *r = (uint8_t)(p * 255); *g = (uint8_t)(q * 255); *b = (uint8_t)(v * 255); break;
        case 4: *r = (uint8_t)(t * 255); *g = (uint8_t)(p * 255); *b = (uint8_t)(v * 255); break;
        default: *r = (uint8_t)(v * 255); *g = (uint8_t)(p * 255); *b = (uint8_t)(q * 255); break;
    }
}
