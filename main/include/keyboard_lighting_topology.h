#ifndef _KEYBOARD_LIGHTING_TOPOLOGY_H_
#define _KEYBOARD_LIGHTING_TOPOLOGY_H_

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "lamp_array/lamp_array_matrix.h"

#ifdef __cplusplus
extern "C" {
#endif

#define YBK_LIGHTING_TOPOLOGY_MAGIC 0x544C4259u /* "YBLT" little-endian */
#define YBK_LIGHTING_TOPOLOGY_VERSION 1
#define YBK_LIGHTING_MAX_LAMPS 80
#define YBK_LIGHTING_NO_KEY 0xFF

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint16_t led_count;
    uint16_t reserved;
    uint8_t led_to_key[YBK_LIGHTING_MAX_LAMPS];
    Position lamp_positions[YBK_LIGHTING_MAX_LAMPS];
} __attribute__((packed)) keyboard_lighting_topology_t;

esp_err_t keyboard_lighting_topology_init(void);
const keyboard_lighting_topology_t *keyboard_lighting_topology_get(void);
esp_err_t keyboard_lighting_topology_set(const keyboard_lighting_topology_t *topology);
void keyboard_lighting_topology_set_default(keyboard_lighting_topology_t *topology);
uint16_t keyboard_lighting_topology_get_led_count(void);
uint8_t keyboard_lighting_topology_key_for_led(uint16_t led_index);
const Position *keyboard_lighting_topology_position_for_led(uint16_t led_index);
bool keyboard_lighting_topology_find_led_for_key(uint8_t key_num, uint16_t *led_index_out);

#ifdef __cplusplus
}
#endif

#endif // _KEYBOARD_LIGHTING_TOPOLOGY_H_
