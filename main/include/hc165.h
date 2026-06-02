#ifndef HC165_H
#define HC165_H

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

// 从 Kconfig 读取配置
#define PIN_DATA    CONFIG_HC165_PIN_DATA
#define PIN_CLK     CONFIG_HC165_PIN_CLK
#define PIN_SHLD    CONFIG_HC165_PIN_SHLD
#define NUM_PRESSED_PINS_MAX CONFIG_HC165_NUM_KEYS

/**
 * @brief 初始化 HC165 按键扫描
 * 
 * 根据 Kconfig 配置自动选择初始化方式：
 * - GPIO 模式：配置 GPIO 引脚
 * - 优化 GPIO 模式：配置 GPIO 引脚
 * - 硬件 SPI 模式：初始化 SPI 总线和设备
 */
void key_init(void);

/**
 * @brief 读取 HC165 按键状态
 * 
 * @param pressed_pins 输出参数，存储被按下的按键编号数组（1-based）
 * @return int 返回按下的按键数量
 * 
 * 根据 Kconfig 配置自动选择读取方式：
 * - GPIO 模式：使用原始 GPIO bit-banging + 延时
 * - 优化 GPIO 模式：使用优化的 GPIO bit-banging（3-4x 快）
 * - 硬件 SPI 模式：使用硬件 SPI 传输（16-20x 快）
 */
int get_pressed_pin(int* pressed_pins);

#endif // HC165_H
