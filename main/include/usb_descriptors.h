/*
 * USB HID 描述符头文件
 * 支持：键盘 + 鼠标 + Lamp Array (Windows 11 动态灯效)
 */

#ifndef _USB_DESCRIPTORS_H_
#define _USB_DESCRIPTORS_H_

#include "tusb.h"

// Report ID 定义
enum {
    REPORT_ID_KEYBOARD = 1,
    REPORT_ID_MOUSE = 2,
    REPORT_ID_CONSUMER_CONTROL = 3,                    // Consumer Control (音量、媒体控制)
    REPORT_ID_LIGHTING_LAMP_ARRAY_ATTRIBUTES = 4,      // Lamp 数组属性
    REPORT_ID_LIGHTING_LAMP_ATTRIBUTES_REQUEST = 5,    // Lamp 属性请求
    REPORT_ID_LIGHTING_LAMP_ATTRIBUTES_RESPONSE = 6,   // Lamp 属性响应
    REPORT_ID_LIGHTING_LAMP_MULTI_UPDATE = 7,          // 多个 Lamp 更新
    REPORT_ID_LIGHTING_LAMP_RANGE_UPDATE = 8,          // 范围 Lamp 更新
    REPORT_ID_LIGHTING_LAMP_ARRAY_CONTROL = 9,         // Lamp 数组控制
    REPORT_ID_COUNT
};

// 注意：TUD_HID_REPORT_DESC_LIGHTING 宏已经在 TinyUSB 库中定义
// 位置：managed_components/espressif__tinyusb/src/class/hid/hid_device.h:496
// 可以直接使用，无需重新定义

#endif // _USB_DESCRIPTORS_H_

