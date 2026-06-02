#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "esp_log.h"
#include "sdkconfig.h"

#define TAG "led_test"

static led_strip_handle_t g_test_strip = NULL;

static uint8_t scale_color(uint8_t value)
{
    uint32_t brightness = CONFIG_LED_TEST_BRIGHTNESS;
    return (uint8_t)((value * brightness) / 255);
}

static void fill_color(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t sr = scale_color(r);
    uint8_t sg = scale_color(g);
    uint8_t sb = scale_color(b);

    for (uint32_t i = 0; i < CONFIG_LED_TEST_NUM_LEDS; i++) {
        led_strip_set_pixel(g_test_strip, i, sr, sg, sb);
    }
    led_strip_refresh(g_test_strip);
}

static void led_test_task(void *pv)
{
    (void)pv;

    led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_LED_TEST_GPIO,
        .max_leds = CONFIG_LED_TEST_NUM_LEDS,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = CONFIG_LED_STRIP_USE_DMA,
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &g_test_strip));
    ESP_LOGI(TAG, "LED strip test started: GPIO=%d, LEDs=%d",
             CONFIG_LED_TEST_GPIO,
             CONFIG_LED_TEST_NUM_LEDS);

    const struct {
        uint8_t r, g, b;
        const char *name;
    } patterns[] = {
        {255,   0,   0, "Red"},
        {  0, 255,   0, "Green"},
        {  0,   0, 255, "Blue"},
        {255, 255, 255, "White"},
    };

    const size_t num_patterns = sizeof(patterns) / sizeof(patterns[0]);

    while (1) {
        for (size_t i = 0; i < num_patterns; i++) {
            ESP_LOGI(TAG, "Displaying %s (brightness=%d/255)", patterns[i].name, CONFIG_LED_TEST_BRIGHTNESS);
            fill_color(patterns[i].r, patterns[i].g, patterns[i].b);
            vTaskDelay(pdMS_TO_TICKS(CONFIG_LED_TEST_PATTERN_DELAY_MS));
        }
    }
}

void app_main(void)
{
    xTaskCreate(led_test_task, "led_test_task", 4096, NULL, 5, NULL);
}

