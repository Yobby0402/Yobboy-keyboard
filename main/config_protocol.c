#include "config_protocol.h"

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "hc165.h"
#include "keyboard_lighting_topology.h"
#include "keyboard_layout_meta.h"
#include "keyboard_power_policy.h"
#include "keyboard_profile.h"

static const char *TAG = "config_protocol";

#define CONFIG_PROTOCOL_TASK_STACK 4096
#define CONFIG_PROTOCOL_TASK_PRIORITY (tskIDLE_PRIORITY + 3)
#define CONFIG_PROTOCOL_READ_CHUNK 128
#define CONFIG_PROTOCOL_MAX_PAYLOAD (sizeof(keyboard_profile_t) + 32)
#define CONFIG_PROTOCOL_RX_BUF_SIZE \
    (sizeof(ybk_config_frame_header_t) + CONFIG_PROTOCOL_MAX_PAYLOAD + sizeof(uint32_t))
#define CONFIG_PROTOCOL_RESPONSE_TYPE(type) ((uint8_t)((type) | 0x80))

static tinyusb_cdcacm_itf_t s_cdc_itf = TINYUSB_CDC_ACM_1;
static uint8_t s_rx_buf[CONFIG_PROTOCOL_RX_BUF_SIZE];
static size_t s_rx_len = 0;
static uint8_t s_external_rx_buf[CONFIG_PROTOCOL_RX_BUF_SIZE];
static size_t s_external_rx_len = 0;
static SemaphoreHandle_t s_tx_lock = NULL;
static vprintf_like_t s_original_vprintf = NULL;
static bool s_log_forward_enabled = false;
static bool s_log_forward_busy = false;

static uint32_t checksum_update(uint32_t hash, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

static uint32_t frame_checksum(const uint8_t *data, size_t len)
{
    return checksum_update(2166136261u, data, len);
}

static uint32_t read_u32_le(const uint8_t *data)
{
    return ((uint32_t)data[0]) |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static void write_u32_le(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value & 0xFF);
    data[1] = (uint8_t)((value >> 8) & 0xFF);
    data[2] = (uint8_t)((value >> 16) & 0xFF);
    data[3] = (uint8_t)((value >> 24) & 0xFF);
}

static ybk_config_status_t status_from_esp_err(esp_err_t err, bool profile_error)
{
    if (err == ESP_OK) {
        return YBK_CONFIG_STATUS_OK;
    }
    if (profile_error || err == ESP_ERR_INVALID_ARG || err == ESP_ERR_INVALID_SIZE ||
        err == ESP_ERR_INVALID_CRC || err == ESP_ERR_INVALID_STATE) {
        return YBK_CONFIG_STATUS_BAD_PROFILE;
    }
    return YBK_CONFIG_STATUS_STORAGE_ERROR;
}

static esp_err_t cdc_write_bytes(const uint8_t *data, size_t len, void *ctx)
{
    (void)ctx;
    if (s_tx_lock != NULL) {
        xSemaphoreTake(s_tx_lock, portMAX_DELAY);
    }
    tinyusb_cdcacm_write_queue(s_cdc_itf, data, len);
    esp_err_t ret = tinyusb_cdcacm_write_flush(s_cdc_itf, pdMS_TO_TICKS(100));
    if (s_tx_lock != NULL) {
        xSemaphoreGive(s_tx_lock);
    }
    return ret;
}

static esp_err_t send_frame_via(ybk_config_transport_write_fn_t write_fn, void *ctx,
                                uint8_t type, uint8_t seq,
                                const uint8_t *payload, uint16_t payload_len)
{
    if (write_fn == NULL || payload_len > CONFIG_PROTOCOL_MAX_PAYLOAD) {
        return ESP_ERR_INVALID_SIZE;
    }

    uint8_t tx_buf[sizeof(ybk_config_frame_header_t) +
                   CONFIG_PROTOCOL_MAX_PAYLOAD + sizeof(uint32_t)];
    ybk_config_frame_header_t header = {
        .magic = YBK_CONFIG_MAGIC,
        .type = type,
        .seq = seq,
        .len = payload_len,
    };

    size_t offset = 0;
    memcpy(tx_buf + offset, &header, sizeof(header));
    offset += sizeof(header);
    if (payload_len > 0 && payload != NULL) {
        memcpy(tx_buf + offset, payload, payload_len);
        offset += payload_len;
    }

    uint32_t checksum = frame_checksum(tx_buf, offset);
    write_u32_le(tx_buf + offset, checksum);
    offset += sizeof(checksum);

    return write_fn(tx_buf, offset, ctx);
}

static esp_err_t send_frame(uint8_t type, uint8_t seq, const uint8_t *payload, uint16_t payload_len)
{
    return send_frame_via(cdc_write_bytes, NULL, type, seq, payload, payload_len);
}

static esp_err_t send_response_via(ybk_config_transport_write_fn_t write_fn, void *ctx,
                                   uint8_t command, uint8_t seq, ybk_config_status_t status,
                                   const uint8_t *payload, uint16_t payload_len)
{
    uint8_t response_payload[1 + CONFIG_PROTOCOL_MAX_PAYLOAD];

    if (payload_len > CONFIG_PROTOCOL_MAX_PAYLOAD) {
        return ESP_ERR_INVALID_SIZE;
    }

    response_payload[0] = status;
    if (payload_len > 0 && payload != NULL) {
        memcpy(response_payload + 1, payload, payload_len);
    }

    return send_frame_via(write_fn, ctx, CONFIG_PROTOCOL_RESPONSE_TYPE(command), seq,
                          response_payload, (uint16_t)(payload_len + 1));
}

static esp_err_t send_response(uint8_t command, uint8_t seq, ybk_config_status_t status,
                               const uint8_t *payload, uint16_t payload_len)
{
    return send_response_via(cdc_write_bytes, NULL, command, seq, status, payload, payload_len);
}

static esp_err_t send_simple_response_via(ybk_config_transport_write_fn_t write_fn, void *ctx,
                                          uint8_t command, uint8_t seq, ybk_config_status_t status)
{
    return send_response_via(write_fn, ctx, command, seq, status, NULL, 0);
}

static esp_err_t send_simple_response(uint8_t command, uint8_t seq, ybk_config_status_t status)
{
    return send_response(command, seq, status, NULL, 0);
}

static void fill_info(ybk_config_info_t *info)
{
    memcpy(info->signature, "YBK1", sizeof(info->signature));
    info->protocol_version = YBK_CONFIG_PROTOCOL_VERSION;
    info->profile_size = sizeof(keyboard_profile_t);
    info->max_keys = YBK_MAX_KEYS;
    info->layer_count = YBK_LAYER_COUNT;
    info->config_cdc_index = s_cdc_itf;
    info->active_profile_checksum = keyboard_profile_get_checksum();
}

static void send_key_state_via(ybk_config_transport_write_fn_t write_fn, void *ctx,
                               uint8_t command, uint8_t seq)
{
    int pressed_pins[NUM_PRESSED_PINS_MAX] = {0};
    int count = get_pressed_pin(pressed_pins);
    ybk_config_key_state_t state = {0};

    if (count < 0) {
        count = 0;
    }
    if (count > YBK_MAX_KEYS) {
        count = YBK_MAX_KEYS;
    }

    state.count = (uint8_t)count;
    for (int i = 0; i < count; i++) {
        state.keys[i] = (uint8_t)pressed_pins[i];
    }

    send_response_via(write_fn, ctx, command, seq,
                      YBK_CONFIG_STATUS_OK, (const uint8_t *)&state, sizeof(state));
}

static void send_key_state(uint8_t command, uint8_t seq)
{
    send_key_state_via(cdc_write_bytes, NULL, command, seq);
}

static void send_runtime_state_via(ybk_config_transport_write_fn_t write_fn, void *ctx,
                                   uint8_t command, uint8_t seq)
{
    const keyboard_power_policy_status_t *status = keyboard_power_policy_get_status();
    ybk_config_runtime_state_t state = {
        .current_mode = (uint8_t)status->current_mode,
        .idle_low_scan_active = status->idle_low_scan_active ? 1 : 0,
        .lighting_paused = status->lighting_paused ? 1 : 0,
        .active_scan_interval_ms = status->active_scan_interval_ms,
        .idle_ms = status->idle_ms,
    };

    send_response_via(write_fn, ctx, command, seq,
                      YBK_CONFIG_STATUS_OK, (const uint8_t *)&state, sizeof(state));
}

static void send_runtime_state(uint8_t command, uint8_t seq)
{
    send_runtime_state_via(cdc_write_bytes, NULL, command, seq);
}

static int config_log_vprintf(const char *fmt, va_list args)
{
    int ret = 0;
    va_list copy;
    va_copy(copy, args);
    if (s_original_vprintf != NULL) {
        ret = s_original_vprintf(fmt, args);
    } else {
        ret = vprintf(fmt, args);
    }

    if (s_log_forward_enabled && !s_log_forward_busy) {
        char line[192];
        int len = vsnprintf(line, sizeof(line), fmt, copy);
        if (len > 0) {
            if (len >= (int)sizeof(line)) {
                len = sizeof(line) - 1;
            }
            s_log_forward_busy = true;
            send_frame(YBK_CONFIG_EVT_LOG, 0, (const uint8_t *)line, (uint16_t)len);
            s_log_forward_busy = false;
        }
    }

    va_end(copy);
    return ret;
}

static void handle_command_via(ybk_config_transport_write_fn_t write_fn, void *ctx,
                               uint8_t command, uint8_t seq,
                               const uint8_t *payload, uint16_t len)
{
    switch (command) {
    case YBK_CONFIG_CMD_GET_INFO: {
        if (len != 0) {
            send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_BAD_LENGTH);
            return;
        }

        ybk_config_info_t info = {0};
        fill_info(&info);
        send_response_via(write_fn, ctx, command, seq,
                          YBK_CONFIG_STATUS_OK, (const uint8_t *)&info, sizeof(info));
        break;
    }

    case YBK_CONFIG_CMD_READ_PROFILE: {
        if (len != 0) {
            send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_BAD_LENGTH);
            return;
        }

        const keyboard_profile_t *profile = keyboard_profile_get();
        send_response_via(write_fn, ctx, command, seq,
                          YBK_CONFIG_STATUS_OK, (const uint8_t *)profile, sizeof(*profile));
        break;
    }

    case YBK_CONFIG_CMD_WRITE_PROFILE: {
        if (len != sizeof(keyboard_profile_t)) {
            send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_BAD_LENGTH);
            return;
        }

        esp_err_t ret = keyboard_profile_stage(payload, len);
        send_simple_response_via(write_fn, ctx, command, seq, status_from_esp_err(ret, true));
        break;
    }

    case YBK_CONFIG_CMD_COMMIT_PROFILE: {
        if (len != 0) {
            send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_BAD_LENGTH);
            return;
        }

        esp_err_t ret = keyboard_profile_commit_staged();
        send_simple_response_via(write_fn, ctx, command, seq,
                                 status_from_esp_err(ret, ret == ESP_ERR_INVALID_STATE));
        break;
    }

    case YBK_CONFIG_CMD_RESET_PROFILE: {
        if (len != 0) {
            send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_BAD_LENGTH);
            return;
        }

        esp_err_t ret = keyboard_profile_reset_default();
        send_simple_response_via(write_fn, ctx, command, seq, status_from_esp_err(ret, false));
        break;
    }

    case YBK_CONFIG_CMD_PREVIEW_LED: {
        if (len != sizeof(ybk_config_led_preview_t)) {
            send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_BAD_LENGTH);
            return;
        }

        ybk_config_led_preview_t preview;
        memcpy(&preview, payload, sizeof(preview));
        keyboard_profile_preview_lighting(preview.mode,
                                          preview.enabled != 0,
                                          preview.brightness,
                                          preview.speed,
                                          preview.red,
                                          preview.green,
                                          preview.blue);
        send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_OK);
        break;
    }

    case YBK_CONFIG_CMD_PREVIEW_LIGHTING_PRESET: {
        if (len != sizeof(keyboard_lighting_preset_t)) {
            send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_BAD_LENGTH);
            return;
        }

        keyboard_lighting_preset_t preset;
        memcpy(&preset, payload, sizeof(preset));
        keyboard_profile_preview_lighting_preset(&preset);
        send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_OK);
        break;
    }

    case YBK_CONFIG_CMD_READ_LIGHTING_TOPOLOGY: {
        if (len != 0) {
            send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_BAD_LENGTH);
            return;
        }

        const keyboard_lighting_topology_t *topology = keyboard_lighting_topology_get();
        send_response_via(write_fn, ctx, command, seq,
                          YBK_CONFIG_STATUS_OK, (const uint8_t *)topology, sizeof(*topology));
        break;
    }

    case YBK_CONFIG_CMD_WRITE_LIGHTING_TOPOLOGY: {
        if (len != sizeof(keyboard_lighting_topology_t)) {
            send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_BAD_LENGTH);
            return;
        }

        keyboard_lighting_topology_t topology;
        memcpy(&topology, payload, sizeof(topology));
        esp_err_t ret = keyboard_lighting_topology_set(&topology);
        if (ret == ESP_OK) {
            keyboard_profile_apply_lighting_runtime();
        }
        send_simple_response_via(write_fn, ctx, command, seq,
                                 status_from_esp_err(ret, ret == ESP_ERR_INVALID_ARG));
        break;
    }

    case YBK_CONFIG_CMD_PREVIEW_LED_INDEX: {
        if (len != sizeof(ybk_config_led_index_preview_t)) {
            send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_BAD_LENGTH);
            return;
        }

        ybk_config_led_index_preview_t preview;
        memcpy(&preview, payload, sizeof(preview));
        keyboard_profile_preview_led_index(preview.led_index,
                                           preview.brightness,
                                           preview.red,
                                           preview.green,
                                           preview.blue);
        send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_OK);
        break;
    }

    case YBK_CONFIG_CMD_REBOOT:
        if (len != 0) {
            send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_BAD_LENGTH);
            return;
        }

        send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_OK);
        vTaskDelay(pdMS_TO_TICKS(100));
        esp_restart();
        break;

    case YBK_CONFIG_CMD_READ_LAYOUT_META: {
        if (len != 0) {
            send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_BAD_LENGTH);
            return;
        }

        const keyboard_layout_meta_t *meta = keyboard_layout_meta_get();
        send_response_via(write_fn, ctx, command, seq,
                          YBK_CONFIG_STATUS_OK, (const uint8_t *)meta, sizeof(*meta));
        break;
    }

    case YBK_CONFIG_CMD_WRITE_LAYOUT_META: {
        if (len != sizeof(keyboard_layout_meta_t)) {
            send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_BAD_LENGTH);
            return;
        }

        keyboard_layout_meta_t meta;
        memcpy(&meta, payload, sizeof(meta));
        esp_err_t ret = keyboard_layout_meta_set(&meta);
        send_simple_response_via(write_fn, ctx, command, seq,
                                 status_from_esp_err(ret, ret == ESP_ERR_INVALID_ARG));
        break;
    }

    case YBK_CONFIG_CMD_READ_KEY_STATE:
        if (len != 0) {
            send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_BAD_LENGTH);
            return;
        }

        send_key_state_via(write_fn, ctx, command, seq);
        break;

    case YBK_CONFIG_CMD_READ_RUNTIME_STATE:
        if (len != 0) {
            send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_BAD_LENGTH);
            return;
        }

        send_runtime_state_via(write_fn, ctx, command, seq);
        break;

    default:
        send_simple_response_via(write_fn, ctx, command, seq, YBK_CONFIG_STATUS_UNKNOWN_CMD);
        break;
    }
}

static void handle_command(uint8_t command, uint8_t seq, const uint8_t *payload, uint16_t len)
{
    handle_command_via(cdc_write_bytes, NULL, command, seq, payload, len);
}

static void discard_rx_bytes(uint8_t *rx_buf, size_t *rx_len, size_t count)
{
    if (count >= *rx_len) {
        *rx_len = 0;
        return;
    }

    memmove(rx_buf, rx_buf + count, *rx_len - count);
    *rx_len -= count;
}

static void process_rx_buffer_via(uint8_t *rx_buf, size_t *rx_len,
                                  ybk_config_transport_write_fn_t write_fn, void *ctx)
{
    while (*rx_len >= sizeof(ybk_config_frame_header_t)) {
        ybk_config_frame_header_t header;
        memcpy(&header, rx_buf, sizeof(header));

        if (header.magic != YBK_CONFIG_MAGIC) {
            discard_rx_bytes(rx_buf, rx_len, 1);
            continue;
        }

        if (header.len > CONFIG_PROTOCOL_MAX_PAYLOAD) {
            ESP_LOGW(TAG, "Dropping oversized frame: %u", header.len);
            discard_rx_bytes(rx_buf, rx_len, 1);
            continue;
        }

        size_t frame_len = sizeof(header) + header.len + sizeof(uint32_t);
        if (*rx_len < frame_len) {
            return;
        }

        const uint8_t *checksum_ptr = rx_buf + sizeof(header) + header.len;
        uint32_t received_checksum = read_u32_le(checksum_ptr);
        uint32_t calculated_checksum = frame_checksum(rx_buf, sizeof(header) + header.len);

        if (received_checksum == calculated_checksum) {
            handle_command_via(write_fn, ctx, header.type, header.seq,
                               rx_buf + sizeof(header), header.len);
        } else {
            ESP_LOGW(TAG, "Bad config frame checksum: cmd=0x%02x", header.type);
            send_simple_response_via(write_fn, ctx, header.type, header.seq,
                                     YBK_CONFIG_STATUS_BAD_CHECKSUM);
        }

        discard_rx_bytes(rx_buf, rx_len, frame_len);
    }
}

static void config_protocol_task(void *arg)
{
    (void)arg;
    uint8_t read_buf[CONFIG_PROTOCOL_READ_CHUNK];

    ESP_LOGI(TAG, "Config protocol task started on CDC%d", s_cdc_itf);

    while (1) {
        size_t rx_size = 0;
        esp_err_t ret = tinyusb_cdcacm_read(s_cdc_itf, read_buf, sizeof(read_buf), &rx_size);
        if (ret == ESP_OK && rx_size > 0) {
            if (s_rx_len + rx_size > sizeof(s_rx_buf)) {
                ESP_LOGW(TAG, "RX buffer overflow, clearing parser state");
                s_rx_len = 0;
            } else {
                memcpy(s_rx_buf + s_rx_len, read_buf, rx_size);
                s_rx_len += rx_size;
                process_rx_buffer_via(s_rx_buf, &s_rx_len, cdc_write_bytes, NULL);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

esp_err_t config_protocol_start(tinyusb_cdcacm_itf_t cdc_itf)
{
    s_cdc_itf = cdc_itf;
    s_rx_len = 0;
    if (s_tx_lock == NULL) {
        s_tx_lock = xSemaphoreCreateMutex();
    }
    if (s_original_vprintf == NULL) {
        s_original_vprintf = esp_log_set_vprintf(config_log_vprintf);
        s_log_forward_enabled = true;
    }

    BaseType_t task_ret = xTaskCreate(config_protocol_task,
                                      "config_protocol",
                                      CONFIG_PROTOCOL_TASK_STACK,
                                      NULL,
                                      CONFIG_PROTOCOL_TASK_PRIORITY,
                                      NULL);
    return (task_ret == pdPASS) ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t config_protocol_process_external_bytes(const uint8_t *data, size_t len,
                                                 ybk_config_transport_write_fn_t write_fn,
                                                 void *ctx)
{
    if (data == NULL || len == 0 || write_fn == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (len > sizeof(s_external_rx_buf)) {
        return ESP_ERR_INVALID_SIZE;
    }
    if (s_external_rx_len + len > sizeof(s_external_rx_buf)) {
        ESP_LOGW(TAG, "External RX buffer overflow, clearing parser state");
        s_external_rx_len = 0;
    }

    memcpy(s_external_rx_buf + s_external_rx_len, data, len);
    s_external_rx_len += len;
    process_rx_buffer_via(s_external_rx_buf, &s_external_rx_len, write_fn, ctx);
    return ESP_OK;
}
