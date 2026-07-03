# Yobboy Keyboard

[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-111111?style=flat-square)](https://www.espressif.com/en/products/socs/esp32-s3)
[![Firmware](https://img.shields.io/badge/Firmware-USB%20%2F%20BLE%20HID-1f6feb?style=flat-square)](#current-capabilities)
[![Configurator](https://img.shields.io/badge/Configurator-WebSerial%20%2F%20BLE-2da44e?style=flat-square)](#browser-configurator)
[![Roadmap](https://img.shields.io/badge/Roadmap-Custom%20Keyboard%20Suite-7c3aed?style=flat-square)](#roadmap)

[简体中文](./README.zh-CN.md)

Yobboy Keyboard is an open-source product project for custom mechanical keyboards.

It is no longer just a firmware repository. The project is growing into an end-to-end custom keyboard solution that connects layout design, keycap generation, PCB and enclosure manufacturing, firmware, and a browser-based configurator. This repository currently owns the software and firmware layer: stable input, visual configuration, persistent profiles, lighting, power behavior, and game-oriented input features.

The long-term goal is to let any keyboard layout start from a KLE-style description and move step by step toward a manufacturable, configurable, and usable keyboard.

## Product Vision

Custom keyboards are usually split across many separate steps: layout design, keycaps, PCB, enclosure, firmware, configuration software, and debugging tools. Yobboy is intended to connect these steps so that "I want a keyboard shaped like this" can become "this is a keyboard that can be manufactured and used."

The current focus is:

- Build a practical keyboard firmware stack.
- Provide a visual browser configurator.
- Support layout import and management from sources such as KLE.
- Prepare a shared data model and workflow for arbitrary-layout keyboard customization.

## Current Capabilities

This repository contains the Yobboy Keyboard firmware, configuration protocol, and browser configurator.

- **Configurable keyboard firmware**
  Built around ESP32-S3, with USB HID, BLE HID, Fn layer support, media controls, device profiles, and NVS-backed persistent configuration.

- **Browser configurator**
  `web-config/` is a static web app that configures the keyboard through USB CDC / WebSerial or BLE. It can read, edit, and save keymaps, lighting, power settings, and device metadata.

- **Lighting system**
  Supports WS2812 RGB lighting, Windows Dynamic Lighting / Lamp Array, multiple lighting presets, and browser-side preview and tuning.

- **Input behavior tuning**
  Supports configurable scan parameters, WASD SOCD, and WASD Reverse Tap Assist for games that care about movement timing and input resolution.

- **Configuration import and export**
  Supports JSON import/export and KLE-derived visual layout import, making configurations easier to back up, share, and iterate.

## Browser Configurator

The configurator lives in `web-config/`. It is not a generic keyboard web template; it is designed around the Yobboy profile model, layout metadata, lighting system, and input behavior controls.

It currently supports:

- Connecting to the keyboard and reading the active profile.
- Editing Base and Fn layer keymaps.
- Adjusting lighting modes, colors, brightness, and presets.
- Configuring scan behavior, sleep behavior, SOCD, and Reverse Tap Assist.
- Editing keyboard name, USB product name, BLE device name, and layout metadata.
- Importing KLE layouts and importing/exporting JSON configuration.

Run it locally with:

```bash
python tools/serve_configurator.py
```

The script starts a local configurator server, beginning at `http://127.0.0.1:8765/` and moving to the next free port if needed. WebSerial usually requires HTTPS or a localhost environment in the browser.

## Related Projects

Yobboy is intended to become a custom keyboard toolchain, not just a single repository.

| Module | Status | Description |
| --- | --- | --- |
| Keyboard firmware and configurator | Implemented in this repository | Runs the keyboard and manages configuration, lighting, input behavior, and device profiles |
| Keycap generator | Separate repository exists | [Yobby0402/Keycap-Generator](https://github.com/Yobby0402/Keycap-Generator), used to generate keycap models from layout and parameters |
| Arbitrary-layout keyboard customization | In development | Intended to derive PCB, Gerber, enclosure, and CNC/manufacturing files from arbitrary keyboard layouts |

## Roadmap

The intended workflow starts from a keyboard layout and gradually produces a keyboard that can be manufactured and used.

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

Stage meanings:

- **KLE**: Describe the keyboard layout with tools such as Keyboard Layout Editor.
- **Layout**: Convert the external layout into the project's internal layout model, including key positions, sizes, rotation, and numbering.
- **Profile**: Generate keymap, lighting, device name, and layout metadata that the firmware and configurator can understand.
- **Placement Solver**: Derive switch, stabilizer, controller, connector, mounting point, and other component placement from the layout.
- **Routing Solver**: Automatically plan matrix routing, power, data lines, and constraints.
- **PCB Generator**: Generate PCB design files.
- **Gerber**: Export manufacturing files for a PCB factory.
- **CNC / PCB Factory**: Move into PCB prototyping, CNC, or other manufacturing processes.
- **Shell**: Generate or adapt the keyboard enclosure, mounting structure, and assembly space.
- **Software**: Flash firmware and use the configurator to tune keymaps, lighting, and input behavior.
- **Keyboard**: Finish a custom keyboard whose layout, hardware, and software are all controlled by the same workflow.

The software and algorithms for directly generating manufacturing files from arbitrary layouts are still in development. The current route is to first connect layout, profile, keycap generation, and firmware configuration, then continue toward automated PCB, enclosure, and manufacturing-file generation.

## Repository Structure

```text
main/                          ESP32-S3 keyboard firmware
main/include/                  Firmware headers
main/lamp_array/               RGB and Lamp Array support
web-config/                    Browser configurator
tools/serve_configurator.py    Local configurator server script
keyboard_layout_raw.json       Raw KLE layout data
WEB_SERIAL_CONFIG_PROTOCOL.md  Configuration protocol notes
KEYBOARD_LAYOUT.md             Keyboard layout notes
```

## Development

The firmware is built with ESP-IDF:

```bash
idf.py build
idf.py -p COM_PORT flash monitor
```

The configurator can be deployed as a static web app. This repository already includes a GitHub Pages workflow:

```text
.github/workflows/deploy-pages.yml
```

## Project Status

Yobboy Keyboard already has a usable firmware and configurator foundation. The next work is focused on:

- Making the configurator feel more like a complete product than a debug tool.
- Connecting the keycap generator, layout import, and keyboard profile more tightly.
- Advancing the arbitrary-layout-to-PCB/enclosure/manufacturing-file generation algorithms.

The final goal is simple: shorten the distance between sketching a custom keyboard idea and building a working keyboard.
