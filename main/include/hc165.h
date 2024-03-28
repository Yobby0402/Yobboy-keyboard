#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"


#define PIN_DATA    16
#define PIN_CLK     17
#define PIN_SHLD    18

#define NUM_PRESSED_PINS_MAX 80 // 最大按键数量
// #define TIMEOUT_MS  1000

void key_init();
int get_pressed_pin(int* pressed_pins);
