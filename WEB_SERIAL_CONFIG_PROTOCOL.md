# Yobboy WebSerial Configuration Protocol

This firmware exposes one USB CDC ACM port for configuration. The GitHub Pages
configurator should connect to this CDC port. Browsers require
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

The device may also send async event frames on the same CDC channel. These use
their own `type` and do not include the leading status byte.

## Commands

```text
0x01 GET_INFO
0x02 READ_PROFILE
0x03 WRITE_PROFILE
0x04 COMMIT_PROFILE
0x05 RESET_PROFILE
0x06 PREVIEW_LED
0x07 REBOOT
0x08 READ_LAYOUT_META
0x09 WRITE_LAYOUT_META
0x0A READ_KEY_STATE
0x70 LOG_EVENT
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

## Layout Metadata

The keyboard stores only a compact layout identity, not the full KLE JSON:

```text
uint32_t magic        "YBKL" (0x4C4B4259)
uint16_t version      1
uint16_t size         44
char     layout_id[32]
uint32_t layout_hash
```

`layout_id` is a stable name such as `yobboy-80`. `layout_hash` is a 32-bit
hash calculated by the configurator from the visual layout. The page can use
this pair to select a matching local/KLE layout after connecting to the device.

## Key State Readback

`READ_KEY_STATE` returns the current physical matrix scan result from the HC165
chain. The response payload body is:

```text
uint8_t count
uint8_t keys[80]
```

Only the first `count` entries are valid. Key numbers are 1-based and match the
firmware key numbering used by the profile.

## Log Event

When the config CDC channel is open, firmware logs can also be mirrored to the
same port as async event frames:

```text
type = 0x70
seq  = 0
payload = UTF-8 log text
```

This lets the web configurator show debug logs without opening the default UART
console.

## Adding More Lighting Effects

The current profile has these lighting fields:

```text
enabled
mode
brightness
effect_speed
color_r
color_g
color_b
```

Current built-in modes:

```text
0 HID
1 SOLID
2 BLINK
3 RAINBOW
4 WAVE
```

`PREVIEW_LED` now uses this payload:

```text
uint8_t mode
uint8_t enabled
uint8_t brightness
uint8_t speed
uint8_t red
uint8_t green
uint8_t blue
```

The frontend sends the selected `mode`, `speed`, and color values, and the
firmware maps them to a runtime LED effect.

For effects that need more parameters, use a versioned payload or a new command,
for example:

```text
0x0A PREVIEW_LED_EFFECT_V2

uint8_t  effect_id
uint8_t  enabled
uint8_t  brightness
uint8_t  speed
uint8_t  color1_r
uint8_t  color1_g
uint8_t  color1_b
uint8_t  color2_r
uint8_t  color2_g
uint8_t  color2_b
uint16_t flags
```

Then either bump `YBK_PROFILE_VERSION` and extend the profile, or store extended
LED parameters in a separate NVS blob so the keymap profile remains stable.
