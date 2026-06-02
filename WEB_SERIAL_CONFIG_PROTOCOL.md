# Yobboy WebSerial Configuration Protocol

This firmware exposes two USB CDC ACM ports:

- CDC0: debug console, used by ESP-IDF logs/stdout.
- CDC1: configuration channel, used by WebSerial or a desktop configurator.

The GitHub Pages configurator should connect to the config CDC port. Browsers require
HTTPS and a user click before `navigator.serial.requestPort()` can open the device.

## Frame Format

All integer fields are little-endian.

```text
offset  size  field
0       4     magic: "YBK1" (0x314B4259)
4       1     type
5       1     seq
6       2     payload length
8       n     payload
8+n     4     checksum
```

Checksum is FNV-1a 32-bit over the frame header and payload, excluding the checksum
field. Use offset basis `2166136261` and prime `16777619`.

Response frames use `type = request_type | 0x80`. Response payload starts with one
status byte, followed by command-specific data.

## Commands

```text
0x01 GET_INFO
0x02 READ_PROFILE
0x03 WRITE_PROFILE
0x04 COMMIT_PROFILE
0x05 RESET_PROFILE
0x06 PREVIEW_LED
0x07 REBOOT
```

Statuses:

```text
0x00 OK
0x01 UNKNOWN_CMD
0x02 BAD_LENGTH
0x03 BAD_CHECKSUM
0x04 BAD_PROFILE
0x05 STORAGE_ERROR
0x06 INTERNAL_ERROR
```

## Profile Update Flow

1. Send `GET_INFO`; verify `profile_size`, `max_keys`, and `layer_count`.
2. Send `READ_PROFILE`; edit the returned `keyboard_profile_t` blob.
3. Send `WRITE_PROFILE` with the full profile blob. The firmware stages it only.
4. Send `COMMIT_PROFILE` to save the staged profile into NVS.
5. Send `REBOOT` or let the user power-cycle the keyboard.

The firmware accepts a profile with checksum set to zero and will fill it before
validation/storage. A configurator may still compute the checksum itself.

## Action Types

Each key action is four bytes:

```text
uint8_t  type
uint8_t  flags
uint16_t code
```

Action types:

```text
0 NONE
1 HID keyboard key
2 HID keyboard modifier bitmap
3 HID consumer usage
4 Fn layer key
5 LED toggle
6 LED brightness up
7 LED brightness down
8 LED effect next
```

The profile contains two layers and 80 key slots:

```text
keymap[layer][key_number]
```

Layer `0` is the base layer. Layer `1` is the Fn layer.
