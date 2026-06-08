#include "keyboard_power.h"

#include <inttypes.h>
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sdkconfig.h"

static const char *TAG = "keyboard_power";

static keyboard_power_status_t s_power = {
    .wake_gpio_enabled = false,
    .wake_gpio = GPIO_NUM_NC,
    .wake_active_low = true,
    .wake_gpio_valid = false,
    .deep_sleep_gpio_supported = false,
    .idle_light_sleep_ms = 0,
    .idle_deep_sleep_ms = 0,
    .last_activity_us = 0,
    .last_wakeup_cause = ESP_SLEEP_WAKEUP_UNDEFINED,
};

static gpio_int_type_t wake_intr_type(void)
{
    return s_power.wake_active_low ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL;
}

static esp_err_t enable_deep_sleep_wakeup_gpio(void)
{
#if SOC_GPIO_SUPPORT_DEEPSLEEP_WAKEUP
    esp_deepsleep_gpio_wake_up_mode_t mode =
        s_power.wake_active_low ? ESP_GPIO_WAKEUP_GPIO_LOW : ESP_GPIO_WAKEUP_GPIO_HIGH;
    return esp_deep_sleep_enable_gpio_wakeup(1ULL << s_power.wake_gpio, mode);
#elif SOC_PM_SUPPORT_EXT1_WAKEUP
    esp_sleep_ext1_wakeup_mode_t mode =
        s_power.wake_active_low ? ESP_EXT1_WAKEUP_ANY_LOW : ESP_EXT1_WAKEUP_ANY_HIGH;
    return esp_sleep_enable_ext1_wakeup_io(1ULL << s_power.wake_gpio, mode);
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static esp_err_t init_wake_gpio(void)
{
#if CONFIG_KEYBOARD_WAKE_GPIO_ENABLE
    s_power.wake_gpio_enabled = true;
    s_power.wake_gpio = (gpio_num_t)CONFIG_KEYBOARD_WAKE_GPIO_NUM;
    s_power.wake_active_low = CONFIG_KEYBOARD_WAKE_GPIO_ACTIVE_LOW;
    s_power.wake_gpio_valid = esp_sleep_is_valid_wakeup_gpio(s_power.wake_gpio);
    s_power.deep_sleep_gpio_supported = s_power.wake_gpio_valid;

    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << CONFIG_KEYBOARD_WAKE_GPIO_NUM,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = s_power.wake_active_low,
        .pull_down_en = !s_power.wake_active_low,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&cfg), TAG, "wake gpio config failed");

    ESP_LOGI(TAG, "Wake GPIO configured: GPIO%d, active %s, sleep wake valid=%s",
             CONFIG_KEYBOARD_WAKE_GPIO_NUM,
             s_power.wake_active_low ? "LOW" : "HIGH",
             s_power.wake_gpio_valid ? "yes" : "no");
#else
    ESP_LOGI(TAG, "Dedicated wake GPIO disabled");
#endif
    return ESP_OK;
}

esp_err_t keyboard_power_init(void)
{
    s_power.idle_light_sleep_ms = CONFIG_KEYBOARD_IDLE_LIGHT_SLEEP_MS;
    s_power.idle_deep_sleep_ms = CONFIG_KEYBOARD_IDLE_DEEP_SLEEP_MS;
    s_power.last_wakeup_cause = esp_sleep_get_wakeup_cause();
    s_power.last_activity_us = esp_timer_get_time();

    ESP_RETURN_ON_ERROR(init_wake_gpio(), TAG, "wake gpio init failed");

    ESP_LOGI(TAG, "Wakeup cause: %d", s_power.last_wakeup_cause);
    ESP_LOGI(TAG, "Idle thresholds: light=%" PRIu32 " ms, deep=%" PRIu32 " ms",
             s_power.idle_light_sleep_ms, s_power.idle_deep_sleep_ms);
    return ESP_OK;
}

void keyboard_power_note_activity(void)
{
    s_power.last_activity_us = esp_timer_get_time();
}

bool keyboard_power_wake_key_pressed(void)
{
    if (!s_power.wake_gpio_enabled) {
        return false;
    }
    int level = gpio_get_level(s_power.wake_gpio);
    return s_power.wake_active_low ? (level == 0) : (level == 1);
}

uint32_t keyboard_power_get_idle_ms(void)
{
    uint64_t now = esp_timer_get_time();
    uint64_t delta_us = (now > s_power.last_activity_us) ? (now - s_power.last_activity_us) : 0;
    return (uint32_t)(delta_us / 1000ULL);
}

bool keyboard_power_should_enter_light_sleep(bool usb_mounted)
{
    if (usb_mounted || s_power.idle_light_sleep_ms == 0) {
        return false;
    }
    return keyboard_power_get_idle_ms() >= s_power.idle_light_sleep_ms;
}

bool keyboard_power_should_enter_deep_sleep(bool usb_mounted)
{
    if (usb_mounted || s_power.idle_deep_sleep_ms == 0) {
        return false;
    }
    return keyboard_power_get_idle_ms() >= s_power.idle_deep_sleep_ms;
}

esp_err_t keyboard_power_prepare_sleep(esp_sleep_mode_t mode)
{
    if (!s_power.wake_gpio_enabled) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (!s_power.wake_gpio_valid) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL), TAG, "disable wake sources failed");

    if (mode == ESP_SLEEP_MODE_LIGHT_SLEEP) {
        ESP_RETURN_ON_ERROR(gpio_wakeup_enable(s_power.wake_gpio, wake_intr_type()), TAG, "gpio wakeup enable failed");
        ESP_RETURN_ON_ERROR(esp_sleep_enable_gpio_wakeup(), TAG, "light sleep gpio wakeup failed");
        ESP_LOGI(TAG, "Prepared light sleep wake source on GPIO%d", s_power.wake_gpio);
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(enable_deep_sleep_wakeup_gpio(), TAG, "deep sleep gpio wakeup failed");
    ESP_LOGI(TAG, "Prepared deep sleep wake source on GPIO%d", s_power.wake_gpio);
    return ESP_OK;
}

esp_err_t keyboard_power_enter_light_sleep(void)
{
    ESP_RETURN_ON_ERROR(keyboard_power_prepare_sleep(ESP_SLEEP_MODE_LIGHT_SLEEP), TAG, "prepare light sleep failed");
    ESP_LOGI(TAG, "Entering light sleep");
    esp_err_t ret = esp_light_sleep_start();
    s_power.last_wakeup_cause = esp_sleep_get_wakeup_cause();
    keyboard_power_note_activity();
    ESP_LOGI(TAG, "Exited light sleep, wakeup cause=%d", s_power.last_wakeup_cause);
    return ret;
}

void keyboard_power_enter_deep_sleep(void)
{
    if (keyboard_power_prepare_sleep(ESP_SLEEP_MODE_DEEP_SLEEP) != ESP_OK) {
        ESP_LOGW(TAG, "Deep sleep prepare failed");
        return;
    }
    ESP_LOGI(TAG, "Entering deep sleep");
    esp_deep_sleep_start();
}

const keyboard_power_status_t *keyboard_power_get_status(void)
{
    return &s_power;
}
