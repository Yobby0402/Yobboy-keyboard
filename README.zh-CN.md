# Yobboy Keyboard

[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-111111?style=flat-square)](https://www.espressif.com/en/products/socs/esp32-s3)
[![USB](https://img.shields.io/badge/USB-HID%20%2B%20CDC-1f6feb?style=flat-square)](#)
[![Configurator](https://img.shields.io/badge/Configurator-WebSerial-7c3aed?style=flat-square)](#)
[![Pages](https://img.shields.io/badge/Deploy-GitHub%20Pages-2da44e?style=flat-square)](#)

[English](./README.md)

这是一个基于 **ESP32-S3** 的开源自定义键盘项目。这个仓库不只是固件本身，也同时包含配置协议、浏览器配置器，以及与这把键盘直接相关的布局与工程文件。

## ✦ 项目简介

Yobboy Keyboard 围绕一套完整的键盘链路展开：

- **ESP32-S3** 作为主控
- **USB HID + CDC** 作为设备接口
- **HC165** 作为矩阵扫描方案
- **NVS 配置持久化** 作为设备侧 profile 存储
- **WebSerial 网页配置器** 作为上位配置入口

它更像是一个完整的键盘项目，而不是一个单独的固件实验仓库：固件、协议、布局元数据和配置页面都属于同一个系统。

## ✦ 仓库包含的内容

- **键盘固件**
- **静态网页配置器**
- **固件与网页共用的配置协议**
- **项目相关的布局与硬件说明**

## ✦ 当前项目特征

目前项目已经包含：

- Fn 双层键位
- 灯光预设与设备预览
- Windows Dynamic Lighting / Lamp Array
- 功耗档位配置
- **WASD SOCD**
- **WASD 反向补键**

## ✦ 仓库结构

```text
main/                          固件主代码
main/include/                  固件头文件
main/lamp_array/               灯光与 Lamp Array 相关代码
web-config/                    静态网页配置器
tools/serve_configurator.py    本地配置器服务器
```

## ✦ 相关文档

- [English README](./README.md)
- [键盘布局说明](./KEYBOARD_LAYOUT.md)
- [DMA / NVS 说明](./DMA_NVS_CONFIG_GUIDE.md)
- [WebSerial 配置协议](./WEB_SERIAL_CONFIG_PROTOCOL.md)

## ✦ 部署

`web-config/` 下的网页配置器可以通过 **GitHub Pages** 发布，工作流文件位于：

- [.github/workflows/deploy-pages.yml](./.github/workflows/deploy-pages.yml)

---

Yobboy Keyboard 是一个体量不大、但链路完整的键盘项目：固件、配置通道和网页工具是一起设计、一起演进的。
