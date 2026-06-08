#include "keyboard_lighting_topology.h"

#include <string.h>
#include "esp_log.h"
#include "keyboard_profile.h"
#include "lightmap.h"
#include "nvs.h"
#include "nvs_config.h"

static const char *TAG = "lighting_topology";

#define LIGHTING_TOPOLOGY_NVS_KEY "led_topology"

static keyboard_lighting_topology_t s_topology;

static void sanitize_topology(keyboard_lighting_topology_t *topology)
{
    if (topology == NULL) {
        return;
    }

    topology->magic = YBK_LIGHTING_TOPOLOGY_MAGIC;
    topology->version = YBK_LIGHTING_TOPOLOGY_VERSION;
    topology->size = sizeof(*topology);
    topology->led_count = LIGHTMAP_NUM;
    topology->reserved = 0;

    for (uint16_t led = 0; led < YBK_LIGHTING_MAX_LAMPS; led++) {
        if (led >= topology->led_count) {
            topology->led_to_key[led] = YBK_LIGHTING_NO_KEY;
            topology->lamp_positions[led] = (Position){0};
            continue;
        }
        if (topology->led_to_key[led] >= YBK_MAX_KEYS) {
            topology->led_to_key[led] = YBK_LIGHTING_NO_KEY;
        }
    }
}

void keyboard_lighting_topology_set_default(keyboard_lighting_topology_t *topology)
{
    if (topology == NULL) {
        return;
    }

    memset(topology, 0, sizeof(*topology));
    topology->magic = YBK_LIGHTING_TOPOLOGY_MAGIC;
    topology->version = YBK_LIGHTING_TOPOLOGY_VERSION;
    topology->size = sizeof(*topology);
    topology->led_count = LIGHTMAP_NUM;
    topology->reserved = 0;
    memcpy(topology->led_to_key, LED_to_Key_Map, sizeof(topology->led_to_key));
    memcpy(topology->lamp_positions, LampPositions, sizeof(topology->lamp_positions));
    sanitize_topology(topology);
}

static bool topology_valid(const keyboard_lighting_topology_t *topology)
{
    return topology != NULL &&
        topology->magic == YBK_LIGHTING_TOPOLOGY_MAGIC &&
        topology->version == YBK_LIGHTING_TOPOLOGY_VERSION &&
        topology->size == sizeof(*topology);
}

static esp_err_t load_topology_from_nvs(keyboard_lighting_topology_t *topology)
{
#ifdef CONFIG_NVS_STORAGE_ENABLE
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        return ret;
    }

    size_t size = sizeof(*topology);
    ret = nvs_get_blob(handle, LIGHTING_TOPOLOGY_NVS_KEY, topology, &size);
    nvs_close(handle);
    if (ret != ESP_OK) {
        return ret;
    }

    if (size != sizeof(*topology) || !topology_valid(topology)) {
        return ESP_ERR_INVALID_SIZE;
    }

    sanitize_topology(topology);
    return ESP_OK;
#else
    (void)topology;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static esp_err_t save_topology_to_nvs(const keyboard_lighting_topology_t *topology)
{
#ifdef CONFIG_NVS_STORAGE_ENABLE
    if (!topology_valid(topology)) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = nvs_set_blob(handle, LIGHTING_TOPOLOGY_NVS_KEY, topology, sizeof(*topology));
    if (ret == ESP_OK) {
        ret = nvs_commit(handle);
    }
    nvs_close(handle);
    return ret;
#else
    (void)topology;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t keyboard_lighting_topology_init(void)
{
    keyboard_lighting_topology_set_default(&s_topology);

    keyboard_lighting_topology_t loaded = {0};
    esp_err_t ret = load_topology_from_nvs(&loaded);
    if (ret == ESP_OK) {
        s_topology = loaded;
        ESP_LOGI(TAG, "Lighting topology loaded: leds=%u", s_topology.led_count);
    } else {
        ESP_LOGW(TAG, "Using default lighting topology: %s", esp_err_to_name(ret));
        (void)save_topology_to_nvs(&s_topology);
    }

    return ESP_OK;
}

const keyboard_lighting_topology_t *keyboard_lighting_topology_get(void)
{
    return &s_topology;
}

esp_err_t keyboard_lighting_topology_set(const keyboard_lighting_topology_t *topology)
{
    if (!topology_valid(topology)) {
        return ESP_ERR_INVALID_ARG;
    }

    s_topology = *topology;
    sanitize_topology(&s_topology);
    return save_topology_to_nvs(&s_topology);
}

uint16_t keyboard_lighting_topology_get_led_count(void)
{
    return s_topology.led_count;
}

uint8_t keyboard_lighting_topology_key_for_led(uint16_t led_index)
{
    if (led_index >= s_topology.led_count || led_index >= YBK_LIGHTING_MAX_LAMPS) {
        return YBK_LIGHTING_NO_KEY;
    }
    return s_topology.led_to_key[led_index];
}

const Position *keyboard_lighting_topology_position_for_led(uint16_t led_index)
{
    if (led_index >= s_topology.led_count || led_index >= YBK_LIGHTING_MAX_LAMPS) {
        return NULL;
    }
    return &s_topology.lamp_positions[led_index];
}

bool keyboard_lighting_topology_find_led_for_key(uint8_t key_num, uint16_t *led_index_out)
{
    for (uint16_t led = 0; led < s_topology.led_count; led++) {
        if (s_topology.led_to_key[led] != key_num) {
            continue;
        }
        if (led_index_out != NULL) {
            *led_index_out = led;
        }
        return true;
    }
    return false;
}
