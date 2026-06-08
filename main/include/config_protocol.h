#ifndef _CONFIG_PROTOCOL_H_
#define _CONFIG_PROTOCOL_H_

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "keyboard_profile.h"
#include "tusb_cdc_acm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define YBK_CONFIG_MAGIC 0x314B4259u /* "YBK1" little-endian */
#define YBK_CONFIG_PROTOCOL_VERSION 3

typedef enum {
    YBK_CONFIG_CMD_GET_INFO = 0x01,
    YBK_CONFIG_CMD_READ_PROFILE = 0x02,
    YBK_CONFIG_CMD_WRITE_PROFILE = 0x03,
    YBK_CONFIG_CMD_COMMIT_PROFILE = 0x04,
    YBK_CONFIG_CMD_RESET_PROFILE = 0x05,
    YBK_CONFIG_CMD_PREVIEW_LED = 0x06,
    YBK_CONFIG_CMD_REBOOT = 0x07,
    YBK_CONFIG_CMD_READ_LAYOUT_META = 0x08,
    YBK_CONFIG_CMD_WRITE_LAYOUT_META = 0x09,
    YBK_CONFIG_CMD_READ_KEY_STATE = 0x0A,
    YBK_CONFIG_CMD_READ_RUNTIME_STATE = 0x0B,
    YBK_CONFIG_CMD_PREVIEW_LIGHTING_PRESET = 0x0C,
    YBK_CONFIG_CMD_READ_LIGHTING_TOPOLOGY = 0x0D,
    YBK_CONFIG_CMD_WRITE_LIGHTING_TOPOLOGY = 0x0E,
    YBK_CONFIG_CMD_PREVIEW_LED_INDEX = 0x0F,
    YBK_CONFIG_EVT_LOG = 0x70,
} ybk_config_command_t;

typedef enum {
    YBK_CONFIG_STATUS_OK = 0x00,
    YBK_CONFIG_STATUS_UNKNOWN_CMD = 0x01,
    YBK_CONFIG_STATUS_BAD_LENGTH = 0x02,
    YBK_CONFIG_STATUS_BAD_CHECKSUM = 0x03,
    YBK_CONFIG_STATUS_BAD_PROFILE = 0x04,
    YBK_CONFIG_STATUS_STORAGE_ERROR = 0x05,
    YBK_CONFIG_STATUS_INTERNAL_ERROR = 0x06,
} ybk_config_status_t;

typedef struct {
    uint32_t magic;
    uint8_t type;
    uint8_t seq;
    uint16_t len;
} __attribute__((packed)) ybk_config_frame_header_t;

typedef struct {
    char signature[4];
    uint16_t protocol_version;
    uint16_t profile_size;
    uint16_t max_keys;
    uint8_t layer_count;
    uint8_t config_cdc_index;
    uint32_t active_profile_checksum;
} __attribute__((packed)) ybk_config_info_t;

typedef struct {
    uint8_t mode;
    uint8_t enabled;
    uint8_t brightness;
    uint8_t speed;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} __attribute__((packed)) ybk_config_led_preview_t;

typedef struct {
    uint8_t count;
    uint8_t keys[YBK_MAX_KEYS];
} __attribute__((packed)) ybk_config_key_state_t;

typedef struct {
    uint8_t current_mode;
    uint8_t idle_low_scan_active;
    uint8_t lighting_paused;
    uint8_t active_scan_interval_ms;
    uint32_t idle_ms;
} __attribute__((packed)) ybk_config_runtime_state_t;

typedef struct {
    uint16_t led_index;
    uint8_t brightness;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} __attribute__((packed)) ybk_config_led_index_preview_t;

typedef esp_err_t (*ybk_config_transport_write_fn_t)(const uint8_t *data, size_t len, void *ctx);

esp_err_t config_protocol_start(tinyusb_cdcacm_itf_t cdc_itf);
esp_err_t config_protocol_process_external_bytes(const uint8_t *data, size_t len,
                                                 ybk_config_transport_write_fn_t write_fn,
                                                 void *ctx);

#ifdef __cplusplus
}
#endif

#endif // _CONFIG_PROTOCOL_H_
