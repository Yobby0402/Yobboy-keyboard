/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 * 
 * 修改: 集成 Windows 11 Lamp Array 支持
 */

#include "sdkconfig.h"

#if !CONFIG_LED_STRIP_TEST_APP

#include <stdlib.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "class/hid/hid_device.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "hc165.h"
#include "led.h"
#include "key_bind.h"
#include "lightmap.h"
#include "lamp_array/lamp_array_matrix.h"
#include "lamp_array/pixel.h"
#include "usb_descriptors.h"
#include "nvs_config.h"
#include "keyboard_profile.h"
#include "keyboard_layout_meta.h"
#include "keyboard_power.h"
#include "keyboard_power_policy.h"
#include "keyboard_transport.h"
#include "config_protocol.h"

#define APP_BUTTON (GPIO_NUM_0)
static const char *TAG = "keyboard";

// LED Strip 句柄
static led_strip_handle_t led_strip_handle = NULL;

// 全局配置
static keyboard_config_t *g_config = NULL;

/************* TinyUSB descriptors ****************/

/**
 * @brief HID report descriptor
 * 包含键盘 + 鼠标 + Consumer Control (音量) + Lamp Array (Windows 11 动态照明)
 */
const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(REPORT_ID_MOUSE)),
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(REPORT_ID_CONSUMER_CONTROL)),
    TUD_HID_REPORT_DESC_LIGHTING(REPORT_ID_LIGHTING_LAMP_ARRAY_ATTRIBUTES)
};

/**
 * @brief String descriptor
 */
const char* hid_string_descriptor[] = {
    (char[]){0x09, 0x04},
    "Yobboy",
    "Yobboy Keyboard",
    "YBK001",
    "Yobboy Keyboard HID",
    "Yobboy Config CDC",
};

/**
 * @brief Configuration descriptor
 */
#define EPNUM_HID_IN 0x81
#define EPNUM_CDC_NOTIF 0x82
#define EPNUM_CDC_OUT 0x03
#define EPNUM_CDC_IN 0x83

enum {
    ITF_NUM_HID = 0,
    ITF_NUM_CDC,
    ITF_NUM_CDC_DATA,
    ITF_NUM_TOTAL,
};

#define YBK_USB_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_CDC_DESC_LEN)

static const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, YBK_USB_CONFIG_TOTAL_LEN,
                         TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 4, false, sizeof(hid_report_descriptor), EPNUM_HID_IN, 16, 10),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 5, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
};

/********* TinyUSB HID callbacks ***************/

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void) instance;
    return hid_report_descriptor;
}

void tud_mount_cb(void)
{
    keyboard_transport_notify_usb_mount();
    ESP_LOGI(TAG, "USB mounted");
}

void tud_umount_cb(void)
{
    keyboard_transport_notify_usb_unmount();
    ESP_LOGI(TAG, "USB unmounted");
}

void tud_suspend_cb(bool remote_wakeup_en)
{
    keyboard_transport_notify_usb_suspend(remote_wakeup_en);
    ESP_LOGI(TAG, "USB suspended, remote wakeup %s",
             remote_wakeup_en ? "enabled" : "disabled");
}

void tud_resume_cb(void)
{
    keyboard_transport_notify_usb_resume();
    ESP_LOGI(TAG, "USB resumed");
}

/**
 * @brief GET_REPORT 回调 - Windows 查询 Lamp 属性
 */
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, 
                               hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    (void) instance;
    (void) reqlen;
    
    ESP_LOGI(TAG, "GET_REPORT: ID=%d, Type=%d", report_id, report_type);
    
    if (report_type != HID_REPORT_TYPE_FEATURE) {
        return 0;
    }
    
    switch (report_id) {
        case REPORT_ID_LIGHTING_LAMP_ARRAY_ATTRIBUTES:
            ESP_LOGI(TAG, "  → Lamp Array Attributes");
            return GetLampArrayAttributesReport(buffer);
            
        case REPORT_ID_LIGHTING_LAMP_ATTRIBUTES_RESPONSE:
            ESP_LOGI(TAG, "  → Lamp Attributes");
            return GetLampAttributesReport(buffer);
            
        default:
            return 0;
    }
}

/**
 * @brief SET_REPORT 回调 - Windows 设置 LED 颜色
 */
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, 
                           hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    (void) instance;
    (void) bufsize;
    
    ESP_LOGI(TAG, "SET_REPORT: ID=%d, Type=%d", report_id, report_type);
    
    if (report_type != HID_REPORT_TYPE_FEATURE) {
        return;
    }
    
    switch (report_id) {
        case REPORT_ID_LIGHTING_LAMP_ATTRIBUTES_REQUEST:
            ESP_LOGI(TAG, "  → Lamp Attributes Request");
            SetLampAttributesId(buffer);
            break;
            
        case REPORT_ID_LIGHTING_LAMP_MULTI_UPDATE:
            ESP_LOGI(TAG, "  → Multi-Lamp Update");
            SetMultipleLamps(buffer);
            break;
            
        case REPORT_ID_LIGHTING_LAMP_RANGE_UPDATE:
            ESP_LOGI(TAG, "  → Lamp Range Update");
            SetLampRange(buffer);
            break;
            
        case REPORT_ID_LIGHTING_LAMP_ARRAY_CONTROL:
            ESP_LOGI(TAG, "  → Autonomous Mode");
            SetAutonomousMode(buffer);
            break;
    }
}

/********* Tasks ***************/

/**
 * @brief 按键扫描任务
 */
void button_task(void *pvParameters)
{
    static int pressed_pins[NUM_PRESSED_PINS_MAX];

    while (1) {
        int num_pressed_pins = get_pressed_pin(pressed_pins);
        if (num_pressed_pins > 0 || keyboard_power_wake_key_pressed()) {
            keyboard_power_note_activity();
            keyboard_power_policy_note_activity();
        }
        process_key_press(pressed_pins, num_pressed_pins);

        uint8_t scan_interval_ms = keyboard_power_policy_get_scan_interval_ms(
            g_config ? g_config->scan_interval_ms : DEFAULT_SCAN_INTERVAL);
        vTaskDelay(pdMS_TO_TICKS(scan_interval_ms));
    }
}

/**
 * @brief Lamp Array 更新任务
 */
void lamp_update_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Lamp update task started");
    
    // 计算刷新间隔（根据 Kconfig 配置）
#ifdef CONFIG_LED_STRIP_REFRESH_RATE
    uint32_t refresh_interval_ms = 1000 / CONFIG_LED_STRIP_REFRESH_RATE;
    ESP_LOGI(TAG, "LED refresh rate: %d Hz (interval: %lu ms)", 
             CONFIG_LED_STRIP_REFRESH_RATE, (unsigned long)refresh_interval_ms);
#else
    uint32_t refresh_interval_ms = 10;  // 默认 100 Hz
#endif
    
    while (1) {
        // 定期刷新 LED（应用 Windows 设置的颜色）
        if (keyboard_power_policy_should_update_lighting()) {
            keyboard_profile_lighting_tick(refresh_interval_ms);
            NeopixelUpdateEffect();
        }
        
        vTaskDelay(pdMS_TO_TICKS(refresh_interval_ms));
    }
}

static bool transport_allows_light_sleep(const keyboard_transport_status_t *transport)
{
    if (transport->ble_connected) {
        return false;
    }

    if (!transport->usb_mounted) {
        return true;
    }

    return transport->usb_suspended && transport->usb_remote_wakeup_enabled;
}

static bool transport_allows_deep_sleep(const keyboard_transport_status_t *transport)
{
    return !transport->usb_mounted && !transport->ble_connected;
}

static void prepare_leds_for_deep_sleep(void)
{
    if (!led_state) {
        return;
    }

    led_state = false;
    NeopixelUpdateEffect();
    ESP_LOGI(TAG, "LEDs turned off before deep sleep");
}

/**
 * @brief 电源管理策略任务
 */
void power_task(void *pvParameters)
{
    (void)pvParameters;

    while (1) {
        const keyboard_transport_status_t *transport = keyboard_transport_get_status();
        const keyboard_power_status_t *power = keyboard_power_get_status();

        keyboard_power_policy_tick(transport);

        if (!power->wake_gpio_enabled || !power->wake_gpio_valid) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (keyboard_power_wake_key_pressed()) {
            vTaskDelay(pdMS_TO_TICKS(250));
            continue;
        }

#if CONFIG_KEYBOARD_AUTO_DEEP_SLEEP_ENABLE
        // Deep sleep only runs when both USB and BLE are fully inactive.
        if (transport_allows_deep_sleep(transport) &&
            keyboard_power_policy_should_enter_deep_sleep(transport)) {
            ESP_LOGI(TAG, "Auto deep sleep triggered after %lu ms idle",
                     (unsigned long)keyboard_power_get_idle_ms());
            prepare_leds_for_deep_sleep();
            keyboard_power_enter_deep_sleep();
        }
#endif

#if CONFIG_KEYBOARD_AUTO_LIGHT_SLEEP_ENABLE
        // Light sleep may also be used during USB suspend, so the device can
        // wake on the dedicated key and then request USB remote wakeup.
        if (transport_allows_light_sleep(transport) &&
            keyboard_power_should_enter_light_sleep(false)) {
            ESP_LOGI(TAG, "Auto light sleep triggered after %lu ms idle",
                     (unsigned long)keyboard_power_get_idle_ms());
            esp_err_t ret = keyboard_power_enter_light_sleep();
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Auto light sleep failed: %s", esp_err_to_name(ret));
                keyboard_power_note_activity();
                keyboard_power_policy_note_activity();
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
            keyboard_power_policy_note_activity();
        }
#endif

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

/**
 * @brief 初始化 LED Strip
 */
static void init_led_strip(void)
{
    ESP_LOGI(TAG, "Initializing LED Strip on GPIO %d", LIGHTMAP_GPIO);
    ESP_LOGI(TAG, "Total LEDs: %d", LIGHTMAP_NUM);
    
    // LED Strip 配置
    led_strip_config_t strip_config = {
        .strip_gpio_num = LIGHTMAP_GPIO,
        .max_leds = LIGHTMAP_NUM,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };
    
    // RMT 配置（支持 Kconfig DMA 选项）
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10000000,  // 10MHz
#ifdef CONFIG_LED_STRIP_USE_DMA
        .flags.with_dma = true,     // 使用 DMA（减少 CPU 占用 60%）
#else
        .flags.with_dma = false,    // 不使用 DMA
#endif
    };
    
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip_handle));
    
#ifdef CONFIG_LED_STRIP_USE_DMA
    ESP_LOGI(TAG, "LED DMA: Enabled (High performance mode)");
#else
    ESP_LOGI(TAG, "LED DMA: Disabled (Standard mode)");
#endif
    
    // 默认关闭 LED，等待用户组合键开启或 Windows 控制
    led_strip_clear(led_strip_handle);
    led_strip_refresh(led_strip_handle);
    
    ESP_LOGI(TAG, "LED Strip initialized (default OFF, Windows control only)");
}

static void init_usb_cdc_interfaces(void)
{
    const tinyusb_config_cdcacm_t config_cdc_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_0,
        .callback_rx = NULL,
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL,
    };
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&config_cdc_cfg));
    ESP_ERROR_CHECK(config_protocol_start(TINYUSB_CDC_ACM_0));
}

/**
 * @brief 主程序
 */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Yobboy Keyboard with Windows 11 Lamp Array");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Features:");
    ESP_LOGI(TAG, "  • HC165 key scanning");
    ESP_LOGI(TAG, "  • USB HID Keyboard + Mouse + Volume Control");
    ESP_LOGI(TAG, "  • %d RGB LEDs (WS2812)", LIGHTMAP_NUM);
    ESP_LOGI(TAG, "  • Windows 11 Dynamic Lighting");
#ifdef CONFIG_NVS_STORAGE_ENABLE
    ESP_LOGI(TAG, "  • NVS Configuration Storage");
#endif
#ifdef CONFIG_LED_STRIP_USE_DMA
    ESP_LOGI(TAG, "  • LED DMA Support (High Performance)");
#endif
    ESP_LOGI(TAG, "========================================");
    
    // 初始化 NVS 配置
    ESP_LOGI(TAG, "Initializing NVS configuration...");
    ESP_ERROR_CHECK(nvs_config_init());
    g_config = nvs_config_get_global();
    ESP_LOGI(TAG, "NVS configuration loaded");
    ESP_LOGI(TAG, "  LED Brightness: %d%%", g_config->led_brightness);
    ESP_LOGI(TAG, "  LED Effect: %d", g_config->led_effect);
    ESP_LOGI(TAG, "  LED Enabled: %s", g_config->led_enabled ? "Yes" : "No");
    
    // 加载 LED 状态（默认关闭，仅 Windows 控制）
    led_load_config_from_nvs();
    ESP_LOGI(TAG, "Initializing keyboard runtime profile...");
    ESP_ERROR_CHECK(keyboard_profile_init());
    ESP_ERROR_CHECK(keyboard_layout_meta_init());
    ESP_ERROR_CHECK(keyboard_transport_init());
    ESP_ERROR_CHECK(keyboard_power_init());
    ESP_ERROR_CHECK(keyboard_power_policy_init());
    ESP_LOGI(TAG, "Keyboard profile checksum: 0x%08lx",
             (unsigned long)keyboard_profile_get_checksum());
    ESP_LOGI(TAG, "LED initial state: %s", led_state ? "ON" : "OFF");
    const keyboard_power_status_t *power_status = keyboard_power_get_status();
    const keyboard_power_profile_t *power_profile = keyboard_profile_get_power_profile();
    const keyboard_power_policy_status_t *policy_status = keyboard_power_policy_get_status();
    ESP_LOGI(TAG, "Power wake GPIO: %s",
             power_status->wake_gpio_enabled ? "configured" : "disabled");
    if (power_status->wake_gpio_enabled) {
        ESP_LOGI(TAG, "  Wake GPIO%d active %s, valid=%s",
                 power_status->wake_gpio,
                 power_status->wake_active_low ? "LOW" : "HIGH",
                 power_status->wake_gpio_valid ? "yes" : "no");
    }
    ESP_LOGI(TAG, "  Idle thresholds: light=%lu ms, deep=%lu ms",
             (unsigned long)power_status->idle_light_sleep_ms,
             (unsigned long)power_status->idle_deep_sleep_ms);
    ESP_LOGI(TAG, "  Scan profiles: game=%u ms, office=%u ms, saver=%u ms, idle=%u ms",
             power_profile->scan_interval_game_ms,
             power_profile->scan_interval_office_ms,
             power_profile->scan_interval_saver_ms,
             power_profile->idle_scan_interval_ms);
    ESP_LOGI(TAG, "  Power mode default=%u, idle low power=%u ms, deep sleep=%lu ms",
             power_profile->default_mode,
             power_profile->idle_enter_low_power_ms,
             (unsigned long)power_profile->idle_enter_deep_sleep_ms);
    ESP_LOGI(TAG, "  Runtime power mode=%u, scan=%u ms",
             policy_status->current_mode,
             policy_status->active_scan_interval_ms);
#if CONFIG_KEYBOARD_AUTO_LIGHT_SLEEP_ENABLE
    ESP_LOGI(TAG, "  Auto light sleep: enabled");
#else
    ESP_LOGI(TAG, "  Auto light sleep: disabled");
#endif
#if CONFIG_KEYBOARD_AUTO_DEEP_SLEEP_ENABLE
    ESP_LOGI(TAG, "  Auto deep sleep: enabled");
#else
    ESP_LOGI(TAG, "  Auto deep sleep: disabled");
#endif
    ESP_LOGI(TAG, "  Sleep policy: light sleep allows USB suspend remote wake, deep sleep requires no active transport");
    
    // Initialize button
    const gpio_config_t boot_button_config = {
        .pin_bit_mask = BIT64(APP_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up_en = true,
        .pull_down_en = false,
    };
    ESP_ERROR_CHECK(gpio_config(&boot_button_config));
    
    // Initialize LED Strip
    init_led_strip();
    
    // Initialize USB
    ESP_LOGI(TAG, "Initializing USB with Lamp Array support");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = hid_string_descriptor,
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
        .external_phy = false,
        .configuration_descriptor = hid_configuration_descriptor,
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    init_usb_cdc_interfaces();
    ESP_LOGI(TAG, "USB initialized");
    
    // Initialize Lamp Array
    ESP_LOGI(TAG, "Initializing Lamp Array Matrix");
    lamp_array_matrix_cfg_t lamp_cfg = {
        .lamp_array_width = LIGHTMAP_WIDTH,
        .lamp_array_height = LIGHTMAP_HEIGHT,
        .lamp_array_depth = LIGHTMAP_DEPTH,
        .lamp_array_rotation = LampPositions,
        .pixel_cnt = LIGHTMAP_NUM,
        .update_interval = LIGHTMAP_UPDATE_INTERVAL,
        .handle = led_strip_handle,
        .bind_key = 0,
    };
    ESP_ERROR_CHECK(lamp_array_matrix_init(lamp_cfg));
    keyboard_profile_apply_lighting_runtime();
    ESP_LOGI(TAG, "Lamp Array initialized: %d lamps", LIGHTMAP_NUM);
    
    // Initialize key scanning
    key_init();
    
    // Create tasks
    xTaskCreate(button_task, "button_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(lamp_update_task, "lamp_task", 4096, NULL, configMAX_PRIORITIES - 2, NULL);
    xTaskCreate(power_task, "power_task", 3072, NULL, configMAX_PRIORITIES - 3, NULL);
    
    // NOTE: 原有的 led_task 已被 lamp_update_task 替代
    // LED 现在由 Windows Lamp Array 控制，或者在自主模式下由 pixel.c 控制
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "System ready!");
    ESP_LOGI(TAG, "Windows: Settings → Personalization → Dynamic Lighting");
    ESP_LOGI(TAG, "Device: Yobboy Keyboard");
    ESP_LOGI(TAG, "========================================");
    
    vTaskDelete(NULL);
}

#endif // !CONFIG_LED_STRIP_TEST_APP
