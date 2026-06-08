#ifndef _KEYBOARD_LAYOUT_META_H_
#define _KEYBOARD_LAYOUT_META_H_

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define YBK_LAYOUT_META_MAGIC 0x4C4B4259u /* "YBKL" little-endian */
#define YBK_LAYOUT_META_VERSION 3
#define YBK_LAYOUT_ID_LEN 32
#define YBK_KEYBOARD_NAME_LEN 32
#define YBK_USB_PRODUCT_NAME_LEN 32
#define YBK_BLE_DEVICE_NAME_LEN 32

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    char layout_id[YBK_LAYOUT_ID_LEN];
    char keyboard_name[YBK_KEYBOARD_NAME_LEN];
    uint32_t layout_hash;
    char usb_product_name[YBK_USB_PRODUCT_NAME_LEN];
    char ble_device_name[YBK_BLE_DEVICE_NAME_LEN];
} __attribute__((packed)) keyboard_layout_meta_t;

esp_err_t keyboard_layout_meta_init(void);
const keyboard_layout_meta_t *keyboard_layout_meta_get(void);
esp_err_t keyboard_layout_meta_set(const keyboard_layout_meta_t *meta);
void keyboard_layout_meta_set_default(keyboard_layout_meta_t *meta);

#ifdef __cplusplus
}
#endif

#endif // _KEYBOARD_LAYOUT_META_H_
