#include "hc165.h"

void key_init() {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<PIN_CLK) | (1ULL<<PIN_SHLD);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL<<PIN_DATA);
    gpio_config(&io_conf);

    gpio_set_level(PIN_CLK, 0);
    gpio_set_level(PIN_SHLD, 0);
}



int get_pressed_pin(int* pressed_pins) {
    // 清空数组
    memset(pressed_pins, 0, NUM_PRESSED_PINS_MAX * sizeof(int));

    // 以下为你的原始代码
    gpio_set_level(PIN_SHLD, 1);
    gpio_set_level(PIN_SHLD, 0);
    gpio_set_level(PIN_SHLD, 1);
    
    gpio_set_level(PIN_CLK, 1);
    
    int index = 0;
    
    for (int i = 0; i < NUM_PRESSED_PINS_MAX; i++) {  
        if (gpio_get_level(PIN_DATA) != 1) {
            pressed_pins[index++] = i + 1;
        }
        gpio_set_level(PIN_CLK, 0);
        gpio_set_level(PIN_CLK, 1);
    }

    return index; // 返回按下的按键数量
}
