# Yobboy Keyboard Firmware

[中文说明](./README.zh-CN.md)

Open-source custom keyboard firmware for an ESP32-S3 keyboard project with:

- USB HID keyboard support
- USB CDC configuration/debug channel
- WebSerial-based browser configurator
- Persistent keymap, lighting, and power settings in NVS
- Fn layer editing
- SOCD for WASD
- Reverse Tap Assist for WASD

This repository contains both the firmware and the static web configurator.

## Highlights

### Firmware

- ESP32-S3 USB composite device
- HID keyboard + CDC configurator channel
- HC165 matrix scanning
- Dynamic keymap storage in NVS
- Multi-preset lighting configuration
- Windows Dynamic Lighting / Lamp Array integration
- Configurable scan/power profiles
- WASD SOCD
- WASD Reverse Tap Assist

### Web configurator

- WebSerial connection to the device CDC interface
- Read, edit, save, and reboot from the browser
- English / Chinese UI
- KLE layout import
- Base / Fn layer key editing
- Lighting preset editor and live device preview
- Runtime status view
- Change list before save

## Repository layout

```text
main/                              Firmware source
main/include/                      Firmware headers
main/lamp_array/                   Lighting and Lamp Array implementation
web-config/                        Static browser configurator
tools/serve_configurator.py        Local server for the configurator
KEYBOARD_LAYOUT.md                 Keyboard layout notes
DMA_NVS_CONFIG_GUIDE.md            DMA / NVS setup notes
WEB_SERIAL_CONFIG_PROTOCOL.md      Configurator protocol
```

## Quick start

### Firmware

Requirements:

- ESP-IDF 5.4 or newer
- VS Code + ESP-IDF extension recommended

Build and flash:

```powershell
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

If your board exposes `BOOT` and `RST`, you can enter download mode manually when needed.

### Local web configurator

The configurator is a static site. No frontend build step is required.

```powershell
python tools/serve_configurator.py
```

Then open:

```text
http://127.0.0.1:8765/
```

Recommended browsers:

- Microsoft Edge
- Google Chrome

## Using the configurator

1. Connect the keyboard over USB
2. Open the configurator page
3. Click `Connect`
4. Select the configuration CDC port
5. Click `Read` to load the current profile
6. Edit keymap / lighting / power settings
7. Click `Save` to write the profile to NVS

Settings persist after reboot.

## SOCD and Reverse Tap Assist

Both features currently apply only to `W`, `A`, `S`, and `D`.

### SOCD

SOCD resolves opposite-direction conflicts when both keys are physically pressed.

Example:

- hold `A`
- then press `D`
- output switches according to the configured delay and rule

Use SOCD when you want clean handling of simultaneous opposite inputs.

### Reverse Tap Assist

Reverse Tap Assist injects a short opposite-direction tap after you release a movement key.

Example:

- hold `W`
- release `W`
- wait for the configured delay
- briefly tap `S` for the configured duration

Current configurable parameters:

- enable / disable
- tap delay
- tap duration
- randomized timing

Use this for counter-strafe or quick-stop style behavior.

## GitHub Pages deployment

The browser configurator lives in `web-config/` and can be deployed directly to GitHub Pages.

This repository includes a Pages workflow:

- `.github/workflows/deploy-pages.yml`

It publishes `web-config/` automatically when you push to `master` or `main`.

### First-time setup

1. Push the repository to GitHub
2. Open `Settings -> Pages`
3. Set `Source` to `GitHub Actions`
4. Push a commit that includes the workflow
5. Wait for `Deploy Configurator to GitHub Pages` in the `Actions` tab

Expected site URL:

```text
https://<your-username>.github.io/Yobboy-keyboard/
```

## Notes

- GitHub Pages uses HTTPS, which works well with WebSerial
- The browser configurator is for configuration and preview, not firmware flashing
- Firmware flashing is still expected to go through ESP-IDF / esptool
- If your worktree contains large unrelated changes, review them before pushing

## Related documents

- [Chinese README](./README.zh-CN.md)
- [Keyboard layout notes](./KEYBOARD_LAYOUT.md)
- [DMA / NVS guide](./DMA_NVS_CONFIG_GUIDE.md)
- [WebSerial protocol](./WEB_SERIAL_CONFIG_PROTOCOL.md)
