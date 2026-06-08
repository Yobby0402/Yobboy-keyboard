#include "keyboard_layout_meta.h"

#include <string.h>
#include "esp_log.h"
#include "nvs.h"
#include "nvs_config.h"

static const char *TAG = "keyboard_layout_meta";

#define LAYOUT_META_NVS_KEY "layout_meta"
#define DEFAULT_LAYOUT_ID "yobboy-80"
#define DEFAULT_KEYBOARD_NAME "Yobboy 80 Key"
#define DEFAULT_USB_PRODUCT_NAME "Yobboy Keyboard"
#define DEFAULT_BLE_DEVICE_NAME "Yobboy Keyboard BLE"

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    char layout_id[YBK_LAYOUT_ID_LEN];
    uint32_t layout_hash;
} __attribute__((packed)) keyboard_layout_meta_v1_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    char layout_id[YBK_LAYOUT_ID_LEN];
    char keyboard_name[YBK_KEYBOARD_NAME_LEN];
    uint32_t layout_hash;
} __attribute__((packed)) keyboard_layout_meta_v2_t;

static keyboard_layout_meta_t s_layout_meta;

static void sanitize_layout_meta_strings(keyboard_layout_meta_t *meta)
{
    if (meta == NULL) {
        return;
    }

    meta->layout_id[sizeof(meta->layout_id) - 1] = '\0';
    meta->keyboard_name[sizeof(meta->keyboard_name) - 1] = '\0';
    meta->usb_product_name[sizeof(meta->usb_product_name) - 1] = '\0';
    meta->ble_device_name[sizeof(meta->ble_device_name) - 1] = '\0';

    if (meta->keyboard_name[0] == '\0') {
        strncpy(meta->keyboard_name, DEFAULT_KEYBOARD_NAME, sizeof(meta->keyboard_name) - 1);
    }
    if (meta->usb_product_name[0] == '\0') {
        strncpy(meta->usb_product_name, DEFAULT_USB_PRODUCT_NAME, sizeof(meta->usb_product_name) - 1);
    }
    if (meta->ble_device_name[0] == '\0') {
        strncpy(meta->ble_device_name, DEFAULT_BLE_DEVICE_NAME, sizeof(meta->ble_device_name) - 1);
    }
}

void keyboard_layout_meta_set_default(keyboard_layout_meta_t *meta)
{
    memset(meta, 0, sizeof(*meta));
    meta->magic = YBK_LAYOUT_META_MAGIC;
    meta->version = YBK_LAYOUT_META_VERSION;
    meta->size = sizeof(*meta);
    strncpy(meta->layout_id, DEFAULT_LAYOUT_ID, sizeof(meta->layout_id) - 1);
    strncpy(meta->keyboard_name, DEFAULT_KEYBOARD_NAME, sizeof(meta->keyboard_name) - 1);
    meta->layout_hash = 0;
    strncpy(meta->usb_product_name, DEFAULT_USB_PRODUCT_NAME, sizeof(meta->usb_product_name) - 1);
    strncpy(meta->ble_device_name, DEFAULT_BLE_DEVICE_NAME, sizeof(meta->ble_device_name) - 1);
}

static bool keyboard_layout_meta_validate(const keyboard_layout_meta_t *meta)
{
    return meta != NULL &&
           meta->magic == YBK_LAYOUT_META_MAGIC &&
           meta->version == YBK_LAYOUT_META_VERSION &&
           meta->size == sizeof(*meta);
}

static esp_err_t load_layout_meta_from_nvs(keyboard_layout_meta_t *meta)
{
#ifdef CONFIG_NVS_STORAGE_ENABLE
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        return ret;
    }

    size_t size = 0;
    ret = nvs_get_blob(handle, LAYOUT_META_NVS_KEY, NULL, &size);
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }

    if (size == sizeof(*meta)) {
        ret = nvs_get_blob(handle, LAYOUT_META_NVS_KEY, meta, &size);
        nvs_close(handle);
        if (ret != ESP_OK) {
            return ret;
        }

        return keyboard_layout_meta_validate(meta) ? ESP_OK : ESP_ERR_INVALID_ARG;
    }

    if (size == sizeof(keyboard_layout_meta_v1_t)) {
        keyboard_layout_meta_v1_t legacy = {0};
        ret = nvs_get_blob(handle, LAYOUT_META_NVS_KEY, &legacy, &size);
        nvs_close(handle);
        if (ret != ESP_OK) {
            return ret;
        }
        if (legacy.magic != YBK_LAYOUT_META_MAGIC ||
            legacy.version != 1 ||
            legacy.size != sizeof(legacy)) {
            return ESP_ERR_INVALID_ARG;
        }

        keyboard_layout_meta_set_default(meta);
        strncpy(meta->layout_id, legacy.layout_id, sizeof(meta->layout_id) - 1);
        meta->layout_hash = legacy.layout_hash;
        sanitize_layout_meta_strings(meta);
        return ESP_OK;
    }

    if (size == sizeof(keyboard_layout_meta_v2_t)) {
        keyboard_layout_meta_v2_t legacy = {0};
        ret = nvs_get_blob(handle, LAYOUT_META_NVS_KEY, &legacy, &size);
        nvs_close(handle);
        if (ret != ESP_OK) {
            return ret;
        }
        if (legacy.magic != YBK_LAYOUT_META_MAGIC ||
            legacy.version != 2 ||
            legacy.size != sizeof(legacy)) {
            return ESP_ERR_INVALID_ARG;
        }

        keyboard_layout_meta_set_default(meta);
        strncpy(meta->layout_id, legacy.layout_id, sizeof(meta->layout_id) - 1);
        strncpy(meta->keyboard_name, legacy.keyboard_name, sizeof(meta->keyboard_name) - 1);
        meta->layout_hash = legacy.layout_hash;
        sanitize_layout_meta_strings(meta);
        return ESP_OK;
    }

    nvs_close(handle);
    return ESP_ERR_INVALID_SIZE;
#else
    (void)meta;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static esp_err_t save_layout_meta_to_nvs(const keyboard_layout_meta_t *meta)
{
#ifdef CONFIG_NVS_STORAGE_ENABLE
    if (!keyboard_layout_meta_validate(meta)) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = nvs_set_blob(handle, LAYOUT_META_NVS_KEY, meta, sizeof(*meta));
    if (ret == ESP_OK) {
        ret = nvs_commit(handle);
    }
    nvs_close(handle);
    return ret;
#else
    (void)meta;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t keyboard_layout_meta_init(void)
{
    keyboard_layout_meta_set_default(&s_layout_meta);

    keyboard_layout_meta_t loaded;
    esp_err_t ret = load_layout_meta_from_nvs(&loaded);
    if (ret == ESP_OK) {
        s_layout_meta = loaded;
        sanitize_layout_meta_strings(&s_layout_meta);
        ESP_LOGI(TAG, "Layout meta loaded: name=%s usb=%s ble=%s id=%s hash=0x%08lx",
                 s_layout_meta.keyboard_name,
                 s_layout_meta.usb_product_name,
                 s_layout_meta.ble_device_name,
                 s_layout_meta.layout_id,
                 (unsigned long)s_layout_meta.layout_hash);
    } else {
        ESP_LOGW(TAG, "Using default layout meta: %s", esp_err_to_name(ret));
        (void)save_layout_meta_to_nvs(&s_layout_meta);
    }

    return ESP_OK;
}

const keyboard_layout_meta_t *keyboard_layout_meta_get(void)
{
    return &s_layout_meta;
}

esp_err_t keyboard_layout_meta_set(const keyboard_layout_meta_t *meta)
{
    if (!keyboard_layout_meta_validate(meta)) {
        return ESP_ERR_INVALID_ARG;
    }

    s_layout_meta = *meta;
    sanitize_layout_meta_strings(&s_layout_meta);
    return save_layout_meta_to_nvs(&s_layout_meta);
}
