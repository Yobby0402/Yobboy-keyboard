#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "esp_log.h"
#include "esp_err.h"

extern bool led_state;
extern int led_effects;
extern int led_brightness;
extern int num_effects;

// GPIO assignment
#define LED_STRIP_BLINK_GPIO  CONFIG_LED_STRIP_BLINK_GPIO
// Numbers of the LED in the strip
#define LED_STRIP_LED_NUMBERS CONFIG_LED_STRIP_LED_NUMBERS
// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define LED_STRIP_RMT_RES_HZ  (CONFIG_LED_STRIP_RMT_RES_HZ * 1000 * 1000)

#define LED_RAINBOW_HUE_INCREMENT ((float)CONFIG_LED_RAINBOW_HUE_INCREMENT / 1000.0f) // Adjust this value to control the speed of the rainbow effect
#define LED_BREATHING_SPEED CONFIG_LED_BREATHING_SPEED // Adjust this value to control the speed of the breathing effect


void hsv_to_rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b);

void update_led_strip(led_strip_handle_t led_strip, int effect);

void led_task(void *pvParameters);
