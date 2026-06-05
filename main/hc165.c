/*
 * HC165 按键扫描驱动
 * 支持三种读取模式（通过 Kconfig 配置）：
 * 1. GPIO 模式：原始实现，带延时
 * 2. 优化 GPIO 模式：减少延时，直接寄存器操作
 * 3. 硬件 SPI 模式：使用 ESP32-S3 SPI 外设，最快速度
 */

#include <stdbool.h>
#include "hc165.h"
#include "esp_rom_sys.h"
#include "esp_log.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t s_scan_lock = NULL;

#ifdef CONFIG_HC165_MODE_HARDWARE_SPI
#include "driver/spi_master.h"

static spi_device_handle_t spi_handle = NULL;
static const char *TAG = "HC165_SPI";

// 根据配置选择 SPI 主机
#if CONFIG_HC165_SPI_HOST == 2
    #define SPI_HOST_SELECTED SPI2_HOST
#elif CONFIG_HC165_SPI_HOST == 3
    #define SPI_HOST_SELECTED SPI3_HOST
#else
    #error "Invalid HC165_SPI_HOST configuration, must be 2 or 3"
#endif

#define SPI_FREQ_HZ (CONFIG_HC165_SPI_FREQ_MHZ * 1000000)

#endif // CONFIG_HC165_MODE_HARDWARE_SPI


//=============================================================================
// 初始化函数
//=============================================================================

void key_init(void) {
    if (s_scan_lock == NULL) {
        s_scan_lock = xSemaphoreCreateMutex();
    }

#ifdef CONFIG_HC165_MODE_HARDWARE_SPI
    // ======== 硬件 SPI 模式初始化 ========
    ESP_LOGI(TAG, "Initializing HC165 in Hardware SPI mode");
    ESP_LOGI(TAG, "SPI Host: %d, Frequency: %d MHz", CONFIG_HC165_SPI_HOST, CONFIG_HC165_SPI_FREQ_MHZ);
    ESP_LOGI(TAG, "Pins - MISO: %d, SCLK: %d, SHLD: %d", PIN_DATA, PIN_CLK, PIN_SHLD);
    
    // 配置 SPI 总线
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = -1,                  // HC165 只读，不需要 MOSI
        .miso_io_num = PIN_DATA,            // 数据输入（HC165 的 QH 引脚）
        .sclk_io_num = PIN_CLK,             // 时钟
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = (NUM_PRESSED_PINS_MAX + 7) / 8,  // 按键数转字节数
    };
    
    esp_err_t ret = spi_bus_initialize(SPI_HOST_SELECTED, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return;
    }
    
    // 配置 SPI 设备
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_FREQ_HZ,
        .mode = 0,                          // SPI 模式 0 (CPOL=0, CPHA=0)
        .spics_io_num = -1,                 // 不使用硬件 CS
        .queue_size = 1,
        .flags = 0,
        .pre_cb = NULL,
        .post_cb = NULL,
    };
    
    ret = spi_bus_add_device(SPI_HOST_SELECTED, &dev_cfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return;
    }
    
    // 配置 SHLD 引脚（并行加载控制）
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_SHLD),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(PIN_SHLD, 1);  // 默认高电平（串行移位状态）
    
    ESP_LOGI(TAG, "HC165 Hardware SPI initialized successfully");
    
#else
    // ======== GPIO 模式初始化（原始和优化模式共用） ========
    gpio_config_t io_conf;
    
    // 配置 CLK 和 SHLD 为输出
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<PIN_CLK) | (1ULL<<PIN_SHLD);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // 配置 DATA 为输入
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL<<PIN_DATA);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // 初始化引脚状态
    gpio_set_level(PIN_CLK, 0);
    gpio_set_level(PIN_SHLD, 1);
    
    #ifdef CONFIG_HC165_MODE_GPIO
        ESP_LOGI("HC165_GPIO", "Initialized in GPIO mode (with delays)");
    #else
        ESP_LOGI("HC165_GPIO", "Initialized in Optimized GPIO mode");
    #endif
    
#endif
}


//=============================================================================
// 读取函数
//=============================================================================

static int get_pressed_pin_unlocked(int* pressed_pins) {
    // 清空数组
    memset(pressed_pins, 0, NUM_PRESSED_PINS_MAX * sizeof(int));
    
#ifdef CONFIG_HC165_MODE_HARDWARE_SPI
    // ======== 硬件 SPI 模式读取 ========
    
    // 1. 锁存数据：SHLD 拉低再拉高
    gpio_set_level(PIN_SHLD, 0);           // 并行加载使能
    esp_rom_delay_us(1);                   // 等待数据锁存
    gpio_set_level(PIN_SHLD, 1);           // 恢复串行移位
    esp_rom_delay_us(1);                   // 等待第一位数据准备好
    
    // 2. 读取第一个按键（不需要 SPI 时钟）
    bool first_key_pressed = (gpio_get_level(PIN_DATA) == 0);

    // 3. 通过 SPI 读取剩余 (N-1) 个按键数据
    int remaining_bits = NUM_PRESSED_PINS_MAX - 1;
    uint8_t rx_data[32] = {0};  // 最多支持 256 个按键（32 字节）
    
    if (remaining_bits > 0) {
        spi_transaction_t trans = {
            .length = remaining_bits,    // 读取剩余位数
            .rxlength = remaining_bits,
            .rx_buffer = rx_data,
            .tx_buffer = NULL,
            .flags = 0,
        };
        
        esp_err_t ret = spi_device_transmit(spi_handle, &trans);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPI transmit failed: %s", esp_err_to_name(ret));
            return 0;
        }
    }
    
    // 4. 解析按键数据（低电平表示按下）
    int index = 0;
    int key_num = 1;

    if (first_key_pressed) {
        pressed_pins[index++] = key_num;
    }
    key_num++;

    for (int bit_idx = 0; bit_idx < remaining_bits; bit_idx++, key_num++) {
        int byte_index = bit_idx / 8;
        int bit_in_byte = 7 - (bit_idx % 8);
        if (key_num > NUM_PRESSED_PINS_MAX) {
            break;
        }

        if ((rx_data[byte_index] & (1 << bit_in_byte)) == 0) {
            pressed_pins[index++] = key_num;
        }
    }
    
    return index;
    
#elif defined(CONFIG_HC165_MODE_GPIO_OPTIMIZED)
    // ======== 优化 GPIO 模式读取 ========
    
    // 锁存数据（无延时，ESP32-S3 指令执行已足够慢）
    gpio_set_level(PIN_SHLD, 0);
    gpio_set_level(PIN_SHLD, 1);
    
    int index = 0;
    
    // 使用寄存器直接操作，减少函数调用开销
    uint32_t clk_mask = (1 << PIN_CLK);
    
    for (int i = 0; i < NUM_PRESSED_PINS_MAX; i++) {
        // 读取当前位（低电平表示按下）
        if (!gpio_get_level(PIN_DATA)) {
            pressed_pins[index++] = i + 1;
        }
        
        // 时钟脉冲（使用寄存器直接操作）
        GPIO.out_w1ts = clk_mask;  // CLK = 1（写 1 置位）
        GPIO.out_w1tc = clk_mask;  // CLK = 0（写 1 清零）
    }
    
    return index;
    
#else
    // ======== 原始 GPIO 模式读取（带延时） ========
    
    // HC165 并行加载时序：SHLD 拉高锁存数据
    gpio_set_level(PIN_SHLD, 0);
    esp_rom_delay_us(1);  // 延时1微秒，确保数据锁存稳定
    gpio_set_level(PIN_SHLD, 1);
    esp_rom_delay_us(1);  // 延时1微秒，准备串行读取
    
    // 开始串行读取
    int index = 0;
    
    for (int i = 0; i < NUM_PRESSED_PINS_MAX; i++) {  
        // 读取当前数据位（低电平表示按下）
        if (gpio_get_level(PIN_DATA) != 1) {
            pressed_pins[index++] = i + 1;
        }
        
        // 时钟脉冲，移位到下一位数据
        gpio_set_level(PIN_CLK, 1);
        esp_rom_delay_us(1);  // 时钟高电平保持时间
        gpio_set_level(PIN_CLK, 0);
        esp_rom_delay_us(1);  // 时钟低电平保持时间
    }

    return index;
    
#endif
}

int get_pressed_pin(int* pressed_pins) {
    if (s_scan_lock != NULL) {
        xSemaphoreTake(s_scan_lock, portMAX_DELAY);
    }

    int ret = get_pressed_pin_unlocked(pressed_pins);

    if (s_scan_lock != NULL) {
        xSemaphoreGive(s_scan_lock);
    }

    return ret;
}
