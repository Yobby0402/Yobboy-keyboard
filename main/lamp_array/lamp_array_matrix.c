/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * 适配：Yobboy 键盘（79 键）
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_err.h"
#include "esp_check.h"
#include "esp_log.h"
#include "lamp_array_matrix.h"
#include "pixel.h"

static const char *TAG = "lamp_array";

#define LAMPARRAY_KIND 1

typedef struct {
    uint16_t current_lamp_Id;
    lamp_array_matrix_cfg_t cfg;
} lamp_array_matrix_t;

static lamp_array_matrix_t *lamp_array_matrix = NULL;

esp_err_t lamp_array_matrix_init(lamp_array_matrix_cfg_t cfg)
{
    ESP_RETURN_ON_FALSE(lamp_array_matrix == NULL, ESP_ERR_INVALID_STATE, TAG, "lamp_array_matrix already initialized");
    lamp_array_matrix = (lamp_array_matrix_t *)calloc(1, sizeof(lamp_array_matrix_t));
    ESP_RETURN_ON_FALSE(lamp_array_matrix != NULL, ESP_ERR_NO_MEM, TAG, "Failed to allocate lamp_array_matrix");
    lamp_array_matrix->cfg = cfg;

    NeopixelInit(cfg.handle, cfg.pixel_cnt);
    
    ESP_LOGI(TAG, "Lamp Array Matrix initialized: %lu lamps, %lux%lux%lu", 
             (unsigned long)cfg.pixel_cnt, 
             (unsigned long)cfg.lamp_array_width, 
             (unsigned long)cfg.lamp_array_height, 
             (unsigned long)cfg.lamp_array_depth);
    
    return ESP_OK;
}

uint16_t GetLampArrayAttributesReport(uint8_t* buffer)
{
    LampArrayAttributesReport report = {
        .lampCount = lamp_array_matrix->cfg.pixel_cnt,
        .width = lamp_array_matrix->cfg.lamp_array_width,
        .height = lamp_array_matrix->cfg.lamp_array_height,
        .depth = lamp_array_matrix->cfg.lamp_array_depth,
        .lampArrayKind = LAMPARRAY_KIND,
        .minUpdateInterval = lamp_array_matrix->cfg.update_interval,
    };

    memcpy(buffer, &report, sizeof(LampArrayAttributesReport));

    return sizeof(LampArrayAttributesReport);
}

uint16_t GetLampAttributesReport(uint8_t* buffer)
{
    LampAttributesResponseReport report = {
        .lampId = lamp_array_matrix->current_lamp_Id,
        .lampPosition = lamp_array_matrix->cfg.lamp_array_rotation[lamp_array_matrix->current_lamp_Id],
        .updateLatency = lamp_array_matrix->cfg.update_interval,
        .lampPurposes = LAMP_PURPOSE_CONTROL,
        .redLevelCount = 255,
        .greenLevelCount = 255,
        .blueLevelCount = 255,
        .intensityLevelCount = 1,
        .isProgrammable = 1,
        .inputBinding = lamp_array_matrix->cfg.bind_key,
    };

    memcpy(buffer, &report, sizeof(LampAttributesResponseReport));
    
    // 自动递增到下一个 Lamp ID
    lamp_array_matrix->current_lamp_Id = lamp_array_matrix->current_lamp_Id + 1 >= lamp_array_matrix->cfg.pixel_cnt 
                                          ? lamp_array_matrix->current_lamp_Id 
                                          : lamp_array_matrix->current_lamp_Id + 1;

    return sizeof(LampAttributesResponseReport);
}

void SetLampAttributesId(const uint8_t* buffer)
{
    LampAttributesRequestReport* report = (LampAttributesRequestReport*) buffer;
    lamp_array_matrix->current_lamp_Id = report->lampId;
}

void SetMultipleLamps(const uint8_t* buffer)
{
    LampMultiUpdateReport* report = (LampMultiUpdateReport*) buffer;

    ESP_LOGI(TAG, "Setting %u lamps", report->lampCount);
    for (int i = 0; i < report->lampCount && i < 8; i++) {
        NeopixelSetColor(report->lampIds[i], report->colors[i]);
        ESP_LOGI(TAG, "  Lamp %u: RGB(%u,%u,%u)", 
                 report->lampIds[i], 
                 report->colors[i].red, 
                 report->colors[i].green, 
                 report->colors[i].blue);
    }
}

void SetLampRange(const uint8_t* buffer)
{
    LampRangeUpdateReport* report = (LampRangeUpdateReport*) buffer;
    NeopixelSetColorRange(report->lampIdStart, report->lampIdEnd, report->color);
    
    ESP_LOGI(TAG, "Set lamp range %u-%u to RGB(%u,%u,%u)", 
             report->lampIdStart, report->lampIdEnd,
             report->color.red, report->color.green, report->color.blue);
}

void SetAutonomousMode(const uint8_t* buffer)
{
    LampArrayControlReport* report = (LampArrayControlReport*) buffer;
    NeopixelSetEffect(report->autonomousMode ? AUTONOMOUS_LIGHTING_EFFECT : HID, AUTONOMOUS_LIGHTING_COLOR);
    
    ESP_LOGI(TAG, "Autonomous mode: %s", report->autonomousMode ? "ON" : "OFF (Windows control)");
}

