# Yobboy Keyboard

[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-111111?style=flat-square)](https://www.espressif.com/en/products/socs/esp32-s3)
[![Firmware](https://img.shields.io/badge/Firmware-USB%20%2F%20BLE%20HID-1f6feb?style=flat-square)](#当前产品能力)
[![Configurator](https://img.shields.io/badge/Configurator-WebSerial%20%2F%20BLE-2da44e?style=flat-square)](#网页配置器)
[![Roadmap](https://img.shields.io/badge/Roadmap-Custom%20Keyboard%20Suite-7c3aed?style=flat-square)](#roadmap)

[English](./README.md)

Yobboy Keyboard 是一个面向客制化键盘的开源产品项目。

它现在已经不只是一份键盘固件，而是在逐步搭建一套从配列设计、键帽建模、PCB 与外壳加工，到固件和网页配置器的完整解决方案。当前仓库主要承担“软件 + 固件”这一层：让一把自制键盘能够稳定输入、可视化配置、保存 profile，并支持灯效、功耗和游戏输入特性。

更长期的目标，是让任意配列的键盘可以从 KLE 这类布局描述开始，一步步生成可制造、可配置、可迭代的成品键盘。

## 项目定位

客制化键盘通常会被拆成很多分散环节：配列设计、键帽、PCB、外壳、固件、配置软件、调试工具。Yobboy 希望把这些环节连接起来，让“我想要一把这样的键盘”可以更自然地落到“这是可以加工和使用的键盘”。

当前阶段的重点是：

- 做好一套真实可用的键盘固件。
- 提供浏览器里的可视化配置器。
- 支持从 KLE 等布局来源导入并管理键盘布局。
- 为后续“任意配列键盘定制”准备统一的数据模型和工作流。

## 当前产品能力

这个仓库目前包含 Yobboy Keyboard 的固件、配置协议和网页配置器。

- **可配置键盘固件**
  基于 ESP32-S3，支持 USB HID、BLE HID、Fn 层、媒体控制、设备 profile、NVS 持久化配置。

- **浏览器配置器**
  `web-config/` 是一个静态网页应用，可通过 USB CDC / WebSerial 或 BLE 配置键盘。它用于读取、编辑、保存键位、灯效、功耗和设备元数据。

- **灯效系统**
  支持 WS2812 RGB 灯效、Windows Dynamic Lighting / Lamp Array、多个灯效预设，以及网页侧预览和调参。

- **输入体验调校**
  支持可配置扫描参数、WASD SOCD、WASD 反向补键，适合需要精细输入时序的游戏场景。

- **配置导入导出**
  支持 JSON 导入导出，也支持从 KLE 派生的可视化布局导入，便于备份、分享和继续迭代。

## 网页配置器

配置器位于 `web-config/`，它不是通用键盘网页模板，而是围绕 Yobboy profile、布局元数据、灯效和输入策略设计的产品界面。

它当前可以完成：

- 连接键盘并读取当前 profile。
- 编辑 Base / Fn 两层键位。
- 调整灯效模式、颜色、亮度和预设。
- 配置扫描、休眠、SOCD、反向补键等输入行为。
- 编辑键盘名、USB 产品名、BLE 设备名和布局信息。
- 导入 KLE 布局，导出 / 导入 JSON 配置。

本地体验：

```bash
python tools/serve_configurator.py
```

脚本会在本机启动配置器，默认从 `http://127.0.0.1:8765/` 开始寻找可用端口。WebSerial 在浏览器中通常需要 HTTPS 或 localhost 环境。

## 相关项目

Yobboy 的目标不是只维护一个单独仓库，而是逐步形成一套客制化键盘工具链。

| 模块 | 状态 | 说明 |
| --- | --- | --- |
| 键盘固件与配置器 | 已在本仓库实现 | 负责键盘运行、配置、灯效、输入策略和设备 profile |
| 键帽生成器 | 已有独立仓库 | [Yobby0402/Keycap-Generator](https://github.com/Yobby0402/Keycap-Generator)，用于从布局和参数生成键帽模型 |
| 任意配列键盘定制 | 开发中 | 目标是从任意配列推导 PCB、Gerber、外壳 / CNC 等加工文件 |

## Roadmap

完整工作流的目标是从一个键盘布局出发，逐步生成可以生产和使用的键盘。

```text
KLE
↓
Layout
↓
Profile
↓
Placement Solver
↓
Routing Solver
↓
PCB Generator
↓
Gerber
↓
CNC / PCB Factory
↓
Shell
↓
Software
↓
Keyboard
```

各阶段的含义：

- **KLE**：使用 Keyboard Layout Editor 这类工具描述键盘配列。
- **Layout**：把外部配列转换为项目内部布局模型，包括按键位置、尺寸、旋转和编号。
- **Profile**：生成固件和配置器可理解的键位、灯光、设备名和布局元数据。
- **Placement Solver**：根据布局推导开关、卫星轴、控制器、接口、固定点等关键部件的放置。
- **Routing Solver**：自动规划矩阵、电源、数据线和约束关系。
- **PCB Generator**：生成 PCB 设计文件。
- **Gerber**：输出可交付 PCB 工厂的制造文件。
- **CNC / PCB Factory**：进入 PCB 打样、CNC 或其他加工流程。
- **Shell**：生成或适配键盘外壳、定位结构和装配空间。
- **Software**：写入固件，使用配置器调校键位、灯效和输入体验。
- **Keyboard**：完成一把从配列到软件都可控的客制化键盘。

其中，“根据任意配列直接生成加工文件”的软件和算法仍在开发中。当前路线会先把布局、profile、键帽和固件配置链路打通，再继续推进 PCB、外壳和制造文件的自动生成。

## 当前仓库结构

```text
main/                          ESP32-S3 键盘固件
main/include/                  固件头文件
main/lamp_array/               RGB 与 Lamp Array 支持
web-config/                    浏览器配置器
tools/serve_configurator.py    本地配置器启动脚本
keyboard_layout_raw.json       KLE 原始布局数据
WEB_SERIAL_CONFIG_PROTOCOL.md  配置协议说明
KEYBOARD_LAYOUT.md             键盘布局说明
```

## 开发入口

固件基于 ESP-IDF 构建：

```bash
idf.py build
idf.py -p COM端口 flash monitor
```

配置器可以作为静态网页部署，仓库中已有 GitHub Pages 工作流：

```text
.github/workflows/deploy-pages.yml
```

## 项目状态

Yobboy Keyboard 当前已经具备可用的固件与配置器基础。后续重点会放在三件事上：

- 让配置器更像一个完整产品，而不是调试工具。
- 将键帽生成器、布局导入和键盘 profile 更紧密地串起来。
- 推进任意配列到 PCB / 外壳 / 加工文件的自动生成算法。

最终目标很简单：让客制化键盘从“画一个想法”到“做出一把键盘”的距离更短。
