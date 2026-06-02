/*
 * NVS 配置存储模块实现
 */

#include "nvs_config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "sdkconfig.h"
#include <string.h>

static const char *TAG = "nvs_config";

// 全局配置
static keyboard_config_t g_config;
static bool g_config_dirty = false;
static TimerHandle_t g_save_timer = NULL;

// 计算校验和
static uint32_t calculate_checksum(const keyboard_config_t *config) {
    uint32_t sum = 0;
    const uint8_t *data = (const uint8_t *)config;
    size_t size = sizeof(keyboard_config_t) - sizeof(uint32_t); // 不包括 checksum 字段
    
    for (size_t i = 0; i < size; i++) {
        sum += data[i];
    }
    
    return sum ^ 0xDEADBEEF; // 简单的异或混淆
}

// 验证校验和
static bool verify_checksum(const keyboard_config_t *config) {
    uint32_t calculated = calculate_checksum(config);
    return calculated == config->checksum;
}

// 延迟保存定时器回调
static void save_timer_callback(TimerHandle_t xTimer) {
    (void)xTimer;
    
    if (g_config_dirty) {
        ESP_LOGI(TAG, "Auto-saving configuration...");
        nvs_config_flush();
    }
}

esp_err_t nvs_config_init(void) {
#ifdef CONFIG_NVS_STORAGE_ENABLE
    esp_err_t ret;
    
    // 初始化 NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS 分区被截断，需要擦除并重新初始化
        ESP_LOGW(TAG, "NVS partition truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "NVS initialized successfully");
    
    // 创建延迟保存定时器
#ifdef CONFIG_NVS_AUTO_SAVE_DELAY
    g_save_timer = xTimerCreate(
        "nvs_save",
        pdMS_TO_TICKS(CONFIG_NVS_AUTO_SAVE_DELAY * 1000),
        pdFALSE,  // 单次触发
        NULL,
        save_timer_callback
    );
    
    if (g_save_timer == NULL) {
        ESP_LOGW(TAG, "Failed to create save timer, auto-save disabled");
    }
#endif
    
    // 加载配置
    ret = nvs_config_load(&g_config);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No saved configuration found, using defaults");
        nvs_config_set_default(&g_config);
        // 保存默认配置
        nvs_config_save(&g_config);
    } else if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load configuration: %s, using defaults", esp_err_to_name(ret));
        nvs_config_set_default(&g_config);
    }
    
    return ESP_OK;
#else
    ESP_LOGI(TAG, "NVS storage disabled, using default configuration");
    nvs_config_set_default(&g_config);
    return ESP_OK;
#endif
}

void nvs_config_set_default(keyboard_config_t *config) {
    config->led_brightness = DEFAULT_LED_BRIGHTNESS;
    config->led_effect = DEFAULT_LED_EFFECT;
    config->led_enabled = DEFAULT_LED_ENABLED;
    config->volume_step = DEFAULT_VOLUME_STEP;
    config->scan_interval_ms = DEFAULT_SCAN_INTERVAL;
    config->config_version = CONFIG_VERSION;
    config->checksum = calculate_checksum(config);
    
    ESP_LOGI(TAG, "Default configuration set");
}

esp_err_t nvs_config_load(keyboard_config_t *config) {
#ifdef CONFIG_NVS_STORAGE_ENABLE
    nvs_handle_t nvs_handle;
    esp_err_t ret;
    
    // 打开 NVS
    ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 读取配置
    size_t size = sizeof(keyboard_config_t);
    ret = nvs_get_blob(nvs_handle, "config", config, &size);
    nvs_close(nvs_handle);
    
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 验证校验和
    if (!verify_checksum(config)) {
        ESP_LOGW(TAG, "Configuration checksum mismatch, data may be corrupted");
        return ESP_ERR_INVALID_CRC;
    }
    
    // 检查版本兼容性
    if (config->config_version != CONFIG_VERSION) {
        ESP_LOGW(TAG, "Configuration version mismatch (saved: %lu, current: %d)",
                 (unsigned long)config->config_version, CONFIG_VERSION);
        // 可以在这里处理版本升级逻辑
    }
    
    ESP_LOGI(TAG, "Configuration loaded: LED brightness=%d, effect=%d, enabled=%d",
             config->led_brightness, config->led_effect, config->led_enabled);
    
    return ESP_OK;
#else
    nvs_config_set_default(config);
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t nvs_config_save(const keyboard_config_t *config) {
#ifdef CONFIG_NVS_STORAGE_ENABLE
    nvs_handle_t nvs_handle;
    esp_err_t ret;
    
    // 创建可修改的副本以更新校验和
    keyboard_config_t config_copy = *config;
    config_copy.checksum = calculate_checksum(&config_copy);
    
    // 打开 NVS
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 写入配置
    ret = nvs_set_blob(nvs_handle, "config", &config_copy, sizeof(keyboard_config_t));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write configuration: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }
    
    // 提交更改
    ret = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit configuration: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Configuration saved successfully");
    g_config_dirty = false;
    
    return ESP_OK;
#else
    ESP_LOGW(TAG, "NVS storage disabled, configuration not saved");
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t nvs_config_erase_all(void) {
#ifdef CONFIG_NVS_STORAGE_ENABLE
    nvs_handle_t nvs_handle;
    esp_err_t ret;
    
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = nvs_erase_all(nvs_handle);
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs_handle);
    }
    
    nvs_close(nvs_handle);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "All configuration erased");
    }
    
    return ret;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

keyboard_config_t* nvs_config_get_global(void) {
    return &g_config;
}

void nvs_config_mark_dirty(void) {
#ifdef CONFIG_NVS_STORAGE_ENABLE
    g_config_dirty = true;
    
    // 重启延迟保存定时器
    if (g_save_timer != NULL) {
        xTimerReset(g_save_timer, 0);
    }
#endif
}

void nvs_config_flush(void) {
#ifdef CONFIG_NVS_STORAGE_ENABLE
    if (g_config_dirty) {
        nvs_config_save(&g_config);
    }
#endif
}

