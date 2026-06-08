export const ACTION_TYPES = {
  NONE: 0,
  KEY: 1,
  MODIFIER: 2,
  CONSUMER: 3,
  LAYER_FN: 4,
  LED_TOGGLE: 5,
  LED_BRIGHTNESS_UP: 6,
  LED_BRIGHTNESS_DOWN: 7,
  LED_EFFECT_NEXT: 8,
  POWER_MODE_NEXT: 9,
  SOCD_TOGGLE: 10,
  REVERSE_TAP_TOGGLE: 11,
  WASD_ASSIST_TOGGLE: 12,
};

export const LED_MODES = [
  { value: 0, label: "Windows HID" },
  { value: 1, label: "Solid" },
  { value: 2, label: "Sine Breath" },
  { value: 3, label: "Rainbow" },
  { value: 4, label: "Wave" },
  { value: 5, label: "Group Static" },
  { value: 6, label: "Per-Key Static" },
  { value: 7, label: "Key Fade" },
  { value: 8, label: "Ripple" },
];

export const KEYCODES = [
  ["KC_A", 0x04], ["KC_B", 0x05], ["KC_C", 0x06], ["KC_D", 0x07],
  ["KC_E", 0x08], ["KC_F", 0x09], ["KC_G", 0x0A], ["KC_H", 0x0B],
  ["KC_I", 0x0C], ["KC_J", 0x0D], ["KC_K", 0x0E], ["KC_L", 0x0F],
  ["KC_M", 0x10], ["KC_N", 0x11], ["KC_O", 0x12], ["KC_P", 0x13],
  ["KC_Q", 0x14], ["KC_R", 0x15], ["KC_S", 0x16], ["KC_T", 0x17],
  ["KC_U", 0x18], ["KC_V", 0x19], ["KC_W", 0x1A], ["KC_X", 0x1B],
  ["KC_Y", 0x1C], ["KC_Z", 0x1D],
  ["KC_1", 0x1E], ["KC_2", 0x1F], ["KC_3", 0x20], ["KC_4", 0x21],
  ["KC_5", 0x22], ["KC_6", 0x23], ["KC_7", 0x24], ["KC_8", 0x25],
  ["KC_9", 0x26], ["KC_0", 0x27],
  ["KC_ENTER", 0x28], ["KC_ESC", 0x29], ["KC_BSPACE", 0x2A],
  ["KC_TAB", 0x2B], ["KC_SPACE", 0x2C], ["KC_MINUS", 0x2D],
  ["KC_EQUAL", 0x2E], ["KC_LBRACKET", 0x2F], ["KC_RBRACKET", 0x30],
  ["KC_BSLASH", 0x31], ["KC_SEMICOLON", 0x33], ["KC_QUOTE", 0x34],
  ["KC_GRAVE", 0x35], ["KC_COMMA", 0x36], ["KC_DOT", 0x37],
  ["KC_SLASH", 0x38], ["KC_CAPS", 0x39],
  ["KC_F1", 0x3A], ["KC_F2", 0x3B], ["KC_F3", 0x3C], ["KC_F4", 0x3D],
  ["KC_F5", 0x3E], ["KC_F6", 0x3F], ["KC_F7", 0x40], ["KC_F8", 0x41],
  ["KC_F9", 0x42], ["KC_F10", 0x43], ["KC_F11", 0x44], ["KC_F12", 0x45],
  ["KC_PSCREEN", 0x46], ["KC_SCROLLLOCK", 0x47], ["KC_PAUSE", 0x48],
  ["KC_INSERT", 0x49], ["KC_HOME", 0x4A], ["KC_PAGEUP", 0x4B],
  ["KC_DELETE", 0x4C], ["KC_END", 0x4D], ["KC_PAGEDOWN", 0x4E],
  ["KC_RIGHT", 0x4F], ["KC_LEFT", 0x50], ["KC_DOWN", 0x51], ["KC_UP", 0x52],
  ["KC_NUMLOCK", 0x53], ["KC_KP_SLASH", 0x54], ["KC_KP_ASTERISK", 0x55],
  ["KC_KP_MINUS", 0x56], ["KC_KP_PLUS", 0x57], ["KC_KP_ENTER", 0x58],
  ["KC_KP_1", 0x59], ["KC_KP_2", 0x5A], ["KC_KP_3", 0x5B],
  ["KC_KP_4", 0x5C], ["KC_KP_5", 0x5D], ["KC_KP_6", 0x5E],
  ["KC_KP_7", 0x5F], ["KC_KP_8", 0x60], ["KC_KP_9", 0x61],
  ["KC_KP_0", 0x62], ["KC_KP_DOT", 0x63], ["KC_APPLICATION", 0x65],
].map(([label, code]) => ({ label, code }));

export const MODIFIERS = [
  { label: "LCTRL", code: 0x01 },
  { label: "LSHIFT", code: 0x02 },
  { label: "LALT", code: 0x04 },
  { label: "LGUI", code: 0x08 },
  { label: "RCTRL", code: 0x10 },
  { label: "RSHIFT", code: 0x20 },
  { label: "RALT", code: 0x40 },
  { label: "RGUI", code: 0x80 },
];

export const CONSUMERS = [
  { label: "MUTE", code: 0x00E2 },
  { label: "VOLUME_UP", code: 0x00E9 },
  { label: "VOLUME_DOWN", code: 0x00EA },
];

export function actionLabel(action) {
  if (!action) return "--";
  switch (action.type) {
    case ACTION_TYPES.NONE:
      return "--";
    case ACTION_TYPES.KEY:
      return KEYCODES.find((key) => key.code === action.code)?.label.replace("KC_", "") || `KC_${action.code}`;
    case ACTION_TYPES.MODIFIER:
      return MODIFIERS.filter((mod) => (action.code & mod.code) !== 0).map((mod) => mod.label).join("+") || "MOD";
    case ACTION_TYPES.CONSUMER:
      return CONSUMERS.find((item) => item.code === action.code)?.label || `CC_${action.code}`;
    case ACTION_TYPES.LAYER_FN:
      return "FN";
    case ACTION_TYPES.LED_TOGGLE:
      return "LED ON";
    case ACTION_TYPES.LED_BRIGHTNESS_UP:
      return "LED +";
    case ACTION_TYPES.LED_BRIGHTNESS_DOWN:
      return "LED -";
    case ACTION_TYPES.LED_EFFECT_NEXT:
      return "LIGHT NEXT";
    case ACTION_TYPES.POWER_MODE_NEXT:
      return "PWR MODE";
    case ACTION_TYPES.SOCD_TOGGLE:
      return "SOCD TOG";
    case ACTION_TYPES.REVERSE_TAP_TOGGLE:
      return "RTAP TOG";
    case ACTION_TYPES.WASD_ASSIST_TOGGLE:
      return "WASD TOG";
    default:
      return `A${action.type}:${action.code}`;
  }
}
