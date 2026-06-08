# Yobboy Keyboard

[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-111111?style=flat-square)](https://www.espressif.com/en/products/socs/esp32-s3)
[![USB](https://img.shields.io/badge/USB-HID%20%2B%20CDC-1f6feb?style=flat-square)](#capabilities)
[![Lighting](https://img.shields.io/badge/Lighting-Lamp%20Array%20%2B%20Presets-7c3aed?style=flat-square)](#capabilities)
[![Configurator](https://img.shields.io/badge/Configurator-WebSerial-2da44e?style=flat-square)](#browser-configurator)

[Chinese / 中文](./README.zh-CN.md)

**Yobboy Keyboard** is an open-source custom keyboard project built around **ESP32-S3**.  
It combines device firmware, a USB configuration channel, and a browser-based configurator in a single repository.

Rather than treating firmware and tooling as separate projects, this repo keeps the whole keyboard system together: matrix scanning, HID behavior, lighting, persistent profiles, layout metadata, and web configuration all evolve as one stack.

## Overview

The project is centered on a practical embedded keyboard architecture:

- **ESP32-S3** as the controller
- **USB HID + CDC** as the device interface
- **HC165** as the key scanning path
- **WS2812 lighting** with device-side preset support
- **NVS-backed profiles** for persistent configuration
- **WebSerial configurator** for browser-based editing and management

This makes the keyboard configurable without recompiling firmware for day-to-day changes such as keymap, lighting, or power behavior.

## Capabilities

### Keyboard firmware

- USB HID keyboard device
- USB CDC configuration / debug channel
- Keymap stored on device instead of being fixed at compile time
- Base layer and Fn layer support
- Consumer/media actions
- Device-side profile validation and version migration

### Lighting system

- **71 WS2812 LEDs**
- Windows Dynamic Lighting / Lamp Array support
- Multiple lighting presets stored in profile
- Solid, breath, rainbow, wave, static-group, per-key static, key-fade, and ripple style modes
- Browser-side editing with live device preview

### Power and input behavior

- Multiple scan/power profiles
- Configurable scan intervals and idle behavior
- **SOCD for WASD**
- **Reverse Tap Assist for WASD**

These input-side features are useful when the keyboard is used for games that care about timing and movement resolution, while still keeping the behavior configurable rather than hard-coded.

## Browser configurator

The configurator in `web-config/` is a static WebSerial app designed specifically for this keyboard.

It currently supports:

- Reading the current device profile
- Editing keymap, Fn layer, lighting, and power parameters
- Editing keyboard name and visual layout metadata
- Importing KLE-derived layouts
- Previewing lighting on the device
- Exporting and importing configuration as JSON

### What export / import covers

The current JSON export/import flow includes the configurator-managed data model:

- full device `profile`
- visual `layout`
- keyboard name (through layout/configurator state)

That means key bindings, lighting presets, power settings, SOCD, Reverse Tap Assist, and the visual layout can all be exported and imported.

It does **not** represent:

- firmware binaries
- transient runtime state
- serial logs

In other words: it is effectively a full **configuration export/import**, not a full device image backup.

## Key project parameters

These are the main current limits and characteristics exposed by the firmware/configurator model:

- **Profile version:** `v8`
- **Maximum keys in profile:** `80`
- **Logical layers:** `2` (`Base` + `Fn`)
- **Lighting preset slots:** `6`
- **Lighting LED count:** `71`
- **USB configuration interface:** CDC ACM

Those values matter because they define the shape of the configuration protocol and the browser configurator.

## Why this design is useful

This project is set up to make a custom keyboard easier to iterate on after hardware is already built:

- **No firmware rebuild for routine tuning**  
  Keymap, lighting, and power behavior live in device profile data.

- **One repository for both firmware and configuration UX**  
  The protocol and the web tool stay aligned with the firmware.

- **Good fit for deployment through GitHub Pages**  
  The configurator is static and browser-based, which makes distribution simple.

- **Room for hardware-specific behavior**  
  Features like SOCD and Reverse Tap Assist are integrated as part of the keyboard profile rather than bolted on externally.

## Repository structure

```text
main/                          Firmware source
main/include/                  Firmware headers
main/lamp_array/               Lighting and Lamp Array implementation
web-config/                    Static browser configurator
tools/serve_configurator.py    Local configurator server
```

## Related files

- [Chinese README](./README.zh-CN.md)
- [Keyboard layout notes](./KEYBOARD_LAYOUT.md)
- [DMA / NVS notes](./DMA_NVS_CONFIG_GUIDE.md)
- [WebSerial protocol](./WEB_SERIAL_CONFIG_PROTOCOL.md)

## Deployment

The browser configurator under `web-config/` can be published with GitHub Pages through:

- [.github/workflows/deploy-pages.yml](./.github/workflows/deploy-pages.yml)

---

Yobboy Keyboard is a compact but complete keyboard project: the firmware, configuration protocol, and browser tooling are part of the same product, not separate sidecars.
