/*
 * NVS 配置存储模块
 * 用于保存和恢复键盘设置
 */

#ifndef _NVS_CONFIG_H_
#define _NVS_CONFIG_H_

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// NVS 命名空间
#define NVS_NAMESPACE "keyboard_cfg"

// 键盘配置结构
typedef struct {
    // LED 设置
    uint8_t led_brightness;        // LED 亮度 (0-100)
    uint8_t led_effect;            // LED 效果编号
    bool led_enabled;              // LED 开关状态
    
    // 音量设置
    uint8_t volume_step;           // 音量步进 (1-10)
    
    // 系统设置
    uint8_t scan_interval_ms;      // 按键扫描间隔 (ms)
    
    // 版本信息（用于升级兼容性）
    uint32_t config_version;       // 配置版本号
    
    // 校验和（用于验证数据完整性）
    uint32_t checksum;
} keyboard_config_t;

// 默认配置
#define DEFAULT_LED_BRIGHTNESS 100
#define DEFAULT_LED_EFFECT 0
#define DEFAULT_LED_ENABLED false
#define DEFAULT_VOLUME_STEP 1
#define DEFAULT_SCAN_INTERVAL 10
#define CONFIG_VERSION 1

/**
 * @brief 初始化 NVS 存储
 * @return ESP_OK 成功, 其他值表示失败
 */
esp_err_t nvs_config_init(void);

/**
 * @brief 加载配置（从 NVS）
 * @param config 配置结构指针
 * @return ESP_OK 成功, ESP_ERR_NVS_NOT_FOUND 未找到配置（使用默认值）
 */
esp_err_t nvs_config_load(keyboard_config_t *config);

/**
 * @brief 保存配置（到 NVS）
 * @param config 配置结构指针
 * @return ESP_OK 成功, 其他值表示失败
 */
esp_err_t nvs_config_save(const keyboard_config_t *config);

/**
 * @brief 恢复默认配置
 * @param config 配置结构指针
 */
void nvs_config_set_default(keyboard_config_t *config);

/**
 * @brief 擦除所有配置
 * @return ESP_OK 成功, 其他值表示失败
 */
esp_err_t nvs_config_erase_all(void);

/**
 * @brief 获取全局配置指针
 * @return 配置结构指针
 */
keyboard_config_t* nvs_config_get_global(void);

/**
 * @brief 标记配置已更改（延迟保存）
 */
void nvs_config_mark_dirty(void);

/**
 * @brief 立即保存配置（如果有更改）
 */
void nvs_config_flush(void);

#ifdef __cplusplus
}
#endif

#endif // _NVS_CONFIG_H_

