# Yobboy Keyboard

[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-111111?style=flat-square)](https://www.espressif.com/en/products/socs/esp32-s3)
[![USB](https://img.shields.io/badge/USB-HID%20%2B%20CDC-1f6feb?style=flat-square)](#)
[![Configurator](https://img.shields.io/badge/Configurator-WebSerial-7c3aed?style=flat-square)](#)
[![Pages](https://img.shields.io/badge/Deploy-GitHub%20Pages-2da44e?style=flat-square)](#)

[中文](./README.zh-CN.md)

An open-source custom keyboard project built around **ESP32-S3**, combining firmware, USB configuration, and a browser-based configurator in one repository.

## ✦ Overview

Yobboy Keyboard is a custom mechanical keyboard firmware project focused on a modern embedded stack:

- **ESP32-S3** as the controller
- **USB HID + CDC** as the device interface
- **HC165** matrix scanning
- **NVS-backed profiles** for persistent configuration
- **WebSerial configurator** for editing keymap, lighting, and runtime behavior from the browser

It is designed as a keyboard project rather than a generic firmware playground: the firmware, protocol, layout metadata, and configurator all live together and evolve together.

## ✦ What is in this repository

- **Firmware** for the keyboard itself
- **Static web configurator** for browser-based setup
- **Configuration protocol** shared between firmware and web UI
- **Project-specific layout and hardware notes**

## ✦ Project character

This project currently includes:

- Fn layer support
- Lighting presets and device preview
- Windows Dynamic Lighting / Lamp Array integration
- Power profile configuration
- **SOCD** for WASD
- **Reverse Tap Assist** for WASD

## ✦ Repository layout

```text
main/                          Firmware source
main/include/                  Firmware headers
main/lamp_array/               Lighting and Lamp Array code
web-config/                    Static browser configurator
tools/serve_configurator.py    Local configurator server
```

## ✦ Related files

- [Chinese README](./README.zh-CN.md)
- [Keyboard layout notes](./KEYBOARD_LAYOUT.md)
- [DMA / NVS notes](./DMA_NVS_CONFIG_GUIDE.md)
- [WebSerial protocol](./WEB_SERIAL_CONFIG_PROTOCOL.md)

## ✦ Deployment

The browser configurator in `web-config/` can be published with **GitHub Pages** through:

- [.github/workflows/deploy-pages.yml](./.github/workflows/deploy-pages.yml)

---

Yobboy Keyboard is a small but complete keyboard project: firmware, configuration channel, and web tooling all belong to the same system.
