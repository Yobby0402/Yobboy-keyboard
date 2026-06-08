# Yobboy Keyboard

[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-111111?style=flat-square)](https://www.espressif.com/en/products/socs/esp32-s3)
[![USB](https://img.shields.io/badge/USB-HID%20%2B%20CDC-1f6feb?style=flat-square)](#项目能力)
[![Lighting](https://img.shields.io/badge/Lighting-Lamp%20Array%20%2B%20Presets-7c3aed?style=flat-square)](#项目能力)
[![Configurator](https://img.shields.io/badge/Configurator-WebSerial-2da44e?style=flat-square)](#网页配置器)

[English](./README.md)

**Yobboy Keyboard** 是一个基于 **ESP32-S3** 的开源自定义键盘项目。  
这个仓库不只是固件本身，也同时包含 USB 配置通道、浏览器配置器、配置协议，以及与这把键盘直接相关的布局和工程文件。

它的核心思路不是把“固件”和“上位工具”拆成两个彼此独立的项目，而是把矩阵扫描、HID 行为、灯光、持久化配置、布局元数据和网页配置器放在同一条链路里一起演进。

## 项目简介

这个项目围绕一套比较完整的键盘架构展开：

- **ESP32-S3** 作为主控
- **USB HID + CDC** 作为设备接口
- **HC165** 作为按键扫描路径
- **WS2812 灯光** 作为设备侧灯光系统
- **NVS 持久化 profile** 作为配置存储
- **WebSerial 网页配置器** 作为浏览器侧管理入口

这意味着大多数日常调校工作，例如键位、灯光、功耗和输入行为，不需要重新编译固件就能完成。

## 项目能力

### 键盘固件

- USB HID 键盘设备
- USB CDC 配置 / 调试通道
- 键位映射存储在设备 profile 中，而不是写死在固件里
- Base 层和 Fn 层
- 媒体 / Consumer 动作
- 设备侧 profile 校验与版本迁移

### 灯光系统

- **71 颗 WS2812 LED**
- Windows Dynamic Lighting / Lamp Array
- 多灯光预设存储在 profile 中
- 支持常亮、呼吸、彩虹、波浪、分组静态、逐键静态、按键渐灭、涟漪等模式
- 支持浏览器侧编辑和设备预览

### 功耗与输入行为

- 多档扫描 / 功耗参数
- 可配置扫描间隔和空闲行为
- **WASD SOCD**
- **WASD 反向补键**

这些输入行为类能力对游戏场景尤其有价值，同时又保持为“可配置能力”，而不是固件里写死的一次性逻辑。

## 网页配置器

`web-config/` 下的配置器是一个面向这把键盘的静态 WebSerial 应用。

它当前支持：

- 读取设备当前 profile
- 编辑键位、Fn 层、灯光与功耗参数
- 编辑键盘名称和可视布局元数据
- 导入 KLE 派生布局
- 向设备发送灯光预览
- 将配置导出为 JSON
- 从 JSON 导入配置

### 当前导入 / 导出覆盖范围

现有 JSON 导出 / 导入覆盖的是配置器管理的数据模型，也就是：

- 完整的设备 `profile`
- 可视 `layout`
- 键盘名称（通过布局 / 配置器状态保存）

也就是说，下面这些内容目前都支持导出与导入：

- 键位绑定
- Fn 层配置
- 灯光预设
- 功耗参数
- SOCD
- 反向补键
- 可视布局

它**不**包含：

- 固件二进制
- 运行时瞬态状态
- 串口日志

换句话说，它已经基本可以视作“完整配置导入 / 导出”，但不是“整机镜像备份”。

## 关键参数

当前固件 / 配置器模型里最重要的一些参数是：

- **Profile 版本：** `v8`
- **Profile 最大按键数：** `80`
- **逻辑层数：** `2`（`Base` + `Fn`）
- **灯光预设槽位：** `6`
- **LED 数量：** `71`
- **USB 配置通道：** CDC ACM

这些参数决定了配置协议的边界，也决定了网页配置器当前的数据结构。

## 这种设计的好处

这个项目的设计，对一把已经做完硬件的键盘来说有几个明显优点：

- **日常调校不依赖重新编译固件**  
  键位、灯光、功耗和输入行为都在 profile 里。

- **固件与配置工具天然对齐**  
  协议和网页不是外部附属项目，而是同一个系统的一部分。

- **适合通过 GitHub Pages 分发**  
  配置器是静态网页，部署和分享都很轻。

- **适合加入硬件定制行为**  
  像 SOCD、反向补键这类与这把键盘强相关的输入逻辑，可以自然地纳入 profile 系统。

## 仓库结构

```text
main/                          固件主代码
main/include/                  固件头文件
main/lamp_array/               灯光与 Lamp Array 实现
web-config/                    静态网页配置器
tools/serve_configurator.py    本地配置器服务器
```

## 相关文档

- [English README](./README.md)
- [键盘布局说明](./KEYBOARD_LAYOUT.md)
- [DMA / NVS 说明](./DMA_NVS_CONFIG_GUIDE.md)
- [WebSerial 配置协议](./WEB_SERIAL_CONFIG_PROTOCOL.md)

## 部署

`web-config/` 下的网页配置器可以通过 GitHub Pages 发布，工作流位于：

- [.github/workflows/deploy-pages.yml](./.github/workflows/deploy-pages.yml)

---

Yobboy Keyboard 不是单纯的一份键盘固件，而是一套围绕这把键盘展开的完整工程：设备固件、配置协议和浏览器工具属于同一个产品层面。
