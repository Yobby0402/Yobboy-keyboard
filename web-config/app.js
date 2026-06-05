import { ACTION_TYPES, CONSUMERS, KEYCODES, MODIFIERS, actionLabel } from "./keycodes.js";
import { defaultLayout, layoutBounds, parseKle } from "./layout.js";
import {
  COMMAND,
  PROFILE,
  YbkSerial,
  checksumOfProfile,
  cloneProfile,
  createEmptyProfile,
  decodeKeyState,
  decodeLayoutMeta,
  decodeRuntimeState,
  encodeLayoutMeta,
  fnv1a,
  getAction,
  getLighting,
  getLightingKeyColor,
  getLightingKeyEnabled,
  getLightingPreset,
  getPowerSettings,
  lightingBytes,
  lightingPresetBytes,
  ledPreviewPayload,
  setAction,
  setLightingActivePreset,
  setLightingKeyColor,
  setLightingKeyEnabled,
  setLightingAutoCycleEnabled,
  setLightingPreset,
  setPowerSettings,
  validateProfile,
} from "./protocol.js";

const els = {
  connect: document.querySelector("#btn-connect"),
  read: document.querySelector("#btn-read"),
  save: document.querySelector("#btn-save"),
  reboot: document.querySelector("#btn-reboot"),
  importKle: document.querySelector("#btn-import-kle"),
  importJson: document.querySelector("#btn-import-json"),
  exportJson: document.querySelector("#btn-export-json"),
  help: document.querySelector("#btn-help"),
  lang: document.querySelector("#btn-lang"),
  langLabel: document.querySelector("#lang-label"),
  fileJson: document.querySelector("#file-json"),
  deviceName: document.querySelector("#device-name"),
  statusText: document.querySelector("#status-text"),
  statusDot: document.querySelector("#status-dot"),
  console: document.querySelector("#console-mini"),
  canvas: document.querySelector("#keyboard-canvas"),
  layoutName: document.querySelector("#layout-name"),
  keyboardNameInput: document.querySelector("#keyboard-name-input"),
  layoutSource: document.querySelector("#layout-source"),
  layoutNote: document.querySelector("#layout-note"),
  layerSwitch: document.querySelector("#layer-switch"),
  warning: document.querySelector("#unsaved-warning"),
  dirtyDot: document.querySelector("#unsaved-dot"),
  keyId: document.querySelector("#display-key-id"),
  layerId: document.querySelector("#display-layer-id"),
  layerBindingOverview: document.querySelector("#layer-binding-overview"),
  actionType: document.querySelector("#action-type"),
  keyboardEditor: document.querySelector("#keyboard-editor"),
  modifierEditor: document.querySelector("#modifier-editor"),
  consumerEditor: document.querySelector("#consumer-editor"),
  keySearch: document.querySelector("#keycode-search"),
  keySelect: document.querySelector("#keycode-select"),
  openKeyPicker: document.querySelector("#btn-open-key-picker"),
  modifierGrid: document.querySelector("#modifier-grid"),
  consumerSelect: document.querySelector("#consumer-select"),
  keyNumber: document.querySelector("#key-number-input"),
  clearKey: document.querySelector("#btn-clear-key"),
  restoreKey: document.querySelector("#btn-restore-key"),
  applyBinding: document.querySelector("#btn-apply-binding"),
  ledEnabled: document.querySelector("#led-enabled"),
  addLightConfig: document.querySelector("#btn-add-light-config"),
  lightingConfigMeta: document.querySelector("#lighting-config-meta"),
  lightingSelectedMeta: document.querySelector("#lighting-selected-meta"),
  lightingPresetList: document.querySelector("#lighting-preset-list"),
  lightingPresetEmpty: document.querySelector("#lighting-preset-empty"),
  lightingAutoCycle: document.querySelector("#lighting-auto-cycle"),
  lightingCycleInterval: document.querySelector("#lighting-cycle-interval"),
  ledMode: document.querySelector("#led-mode"),
  brightness: document.querySelector("#brightness-slider"),
  brightnessOut: document.querySelector("#brightness-output"),
  speed: document.querySelector("#speed-slider"),
  speedOut: document.querySelector("#speed-output"),
  ledColor: document.querySelector("#led-color"),
  lightingStaticActions: document.querySelector("#lighting-static-actions"),
  staticSelectAll: document.querySelector("#btn-static-select-all"),
  staticClearAll: document.querySelector("#btn-static-clear-all"),
  previewLight: document.querySelector("#btn-preview-light"),
  activateLight: document.querySelector("#btn-activate-light"),
  saveLight: document.querySelector("#btn-save-light"),
  powerModeDefault: document.querySelector("#power-mode-default"),
  powerModeCycle: document.querySelector("#power-mode-cycle"),
  socdEnabled: document.querySelector("#socd-enabled"),
  socdDelay: document.querySelector("#socd-delay-ms"),
  socdRandomize: document.querySelector("#socd-randomize"),
  reverseTapEnabled: document.querySelector("#reverse-tap-enabled"),
  reverseTapDelay: document.querySelector("#reverse-tap-delay-ms"),
  reverseTapDuration: document.querySelector("#reverse-tap-duration-ms"),
  reverseTapRandomize: document.querySelector("#reverse-tap-randomize"),
  scanGame: document.querySelector("#scan-game-ms"),
  scanOffice: document.querySelector("#scan-office-ms"),
  scanSaver: document.querySelector("#scan-saver-ms"),
  scanIdle: document.querySelector("#scan-idle-ms"),
  idleLowPower: document.querySelector("#idle-low-power-ms"),
  idleDeepSleep: document.querySelector("#idle-deep-sleep-ms"),
  runtimeMode: document.querySelector("#runtime-mode"),
  runtimeScan: document.querySelector("#runtime-scan"),
  runtimeIdle: document.querySelector("#runtime-idle"),
  runtimeLighting: document.querySelector("#runtime-lighting"),
  settingsTabs: Array.from(document.querySelectorAll("[data-settings-tab]")),
  settingsPanels: Array.from(document.querySelectorAll("[data-settings-panel]")),
  mapMode: document.querySelector("#btn-map-mode"),
  defaultLayout: document.querySelector("#btn-default-layout"),
  kleDialog: document.querySelector("#kle-dialog"),
  kleInput: document.querySelector("#kle-input"),
  applyKle: document.querySelector("#btn-apply-kle"),
  keyPickerDialog: document.querySelector("#key-picker-dialog"),
  keyPickerGrid: document.querySelector("#key-picker-grid"),
  helpDialog: document.querySelector("#help-dialog"),
  toggleInputTest: document.querySelector("#toggle-input-test"),
  toggleDeviceScan: document.querySelector("#toggle-device-scan"),
  toggleKeyNumber: document.querySelector("#toggle-key-number"),
  toggleBinding: document.querySelector("#toggle-binding"),
  inputTestStatus: document.querySelector("#input-test-status"),
  changeList: document.querySelector("#change-list"),
  changeListEmpty: document.querySelector("#change-list-empty"),
  lightPreviewNote: document.querySelector("#light-preview-note"),
  protocolVersion: document.querySelector("#protocol-version"),
  profileVersion: document.querySelector("#profile-version"),
  profileChecksum: document.querySelector("#profile-checksum"),
  profileKeys: document.querySelector("#profile-keys"),
  profileLayers: document.querySelector("#profile-layers"),
};

const I18N = {
  en: {
    importKle: "Import KLE",
    importJson: "Import JSON",
    exportJson: "Export",
    help: "Help",
    read: "Read",
    save: "Save",
    reboot: "Reboot",
    connect: "Connect",
    disconnect: "Disconnect",
    disconnected: "DISCONNECTED",
    connected: "CONNECTED: ESP32-S3",
    connectFailed: "CONNECT FAILED",
    profile: "Profile",
    checksum: "Checksum",
    keys: "Keys",
    layers: "Layers",
    unsaved: "UNSAVED CHANGES",
    keymap: "Keymap",
    lighting: "Lighting",
    layout: "Layout",
    console: "DEBUG CONSOLE",
    inputTest: "Input Test",
    deviceScan: "Device Scan",
    inputTestIdle: "Press keys while this page is focused.",
    inputTestOff: "Input test is disabled.",
    inputTestDisconnected: "Connect the keyboard to use input test.",
    inputTestActive: "Active",
    inputTestUnmapped: "Unmapped event",
    deviceScanIdle: "Device scan is idle.",
    deviceScanOff: "Device scan is disabled.",
    deviceScanDisconnected: "Connect the keyboard to read matrix state.",
    deviceScanActive: "Matrix",
    keyboardName: "Keyboard Name",
    changeList: "Change List",
    changeListEmpty: "No unsaved changes.",
    changeName: "Keyboard Name",
    changeLighting: "Lighting",
    changeKeyNumber: "Key Number",
    changeBinding: "Binding",
    tabKeymap: "Keymap",
    tabLighting: "Lighting",
    tabPower: "Power",
    lightingPresets: "Lighting Configs",
    addLightingConfig: "Add Config",
    lightingEmpty: "No lighting configs yet. Add one to start.",
    lightingConfigCount: "Configs",
    lightingConfig: "Config",
    selectedLightingConfig: "Selected Config",
    removeLightingConfig: "Remove Config",
    lightingConfigLimit: "All lighting config slots are already in use.",
    lightingAutoCycle: "Auto Cycle",
    lightingAutoCycleHint: "When enabled, the keyboard lighting-switch key pauses or resumes cycling.",
    cycleInterval: "Cycle Interval (s)",
    preset: "Preset",
    activePreset: "Active Preset",
    setActivePreset: "Set As Current",
    selectedPreset: "Selected Preset",
    lightingEditHint: "In Group Static or Per-Key Static, click keys in the keyboard view to edit lighting.",
    lightPreviewNote: "Web preview follows the selected preset. Static modes can also be pushed to the device preview.",
    reactivePreviewHint: "Reactive effects need a real key press on the device after preview is sent.",
    selectAllKeys: "Select All Keys",
    clearAllKeys: "Clear All Keys",
    paintAllKeys: "Paint All Keys",
    clearPaintedKeys: "Clear Painted Keys",
    staticActionSelect: "Click to Select",
    staticActionPaint: "Click to Paint",
    keyNumberShort: "Key #",
    binding: "Binding",
    keyProperties: "KEY PROPERTIES",
    layerBindings: "Layer Bindings",
    actionType: "ACTION TYPE",
    actionNone: "None",
    actionKeyboard: "Keyboard Key",
    actionModifier: "Modifier",
    actionConsumer: "Consumer Control",
    actionFn: "Fn Layer",
    actionLedToggle: "LED Toggle",
    actionLedUp: "LED Brightness Up",
    actionLedDown: "LED Brightness Down",
    actionLedNext: "Lighting Preset Next",
    actionPowerModeNext: "Power Mode Next",
    keycodeSearch: "KEYCODE SEARCH",
    keyPicker: "Keyboard Picker",
    keyPickerTitle: "Choose Keyboard Key",
    modifier: "MODIFIER",
    controlAction: "CONTROL ACTION",
    keyNumber: "KEY NUMBER",
    clearKey: "Clear Key",
    restore: "Restore",
    applyBinding: "Apply Binding",
    currentLayer: "Current",
    editLayer: "Edit",
    baseLayer: "Base",
    fnLayer: "Fn",
    effectMode: "EFFECT MODE",
    brightness: "BRIGHTNESS",
    effectSpeed: "EFFECT SPEED",
    rgbColor: "RGB COLOR",
    previewLighting: "Preview Lighting",
    saveLighting: "Save Lighting to Config",
    powerModes: "Power Modes",
    powerModeDefault: "Default Mode",
    powerModeCycle: "Allow Key Toggle",
    powerModeGame: "Game",
    powerModeOffice: "Office",
    powerModeSaver: "Saver",
    socd: "SOCD",
    socdEnable: "Enable SOCD (WASD)",
    socdDelay: "Trigger Delay (ms)",
    socdRandom: "Randomize 1..N ms",
    socdDescription: "Use SOCD when opposite directions may be held together. Example: hold A, then press D, and the output switches to D after the configured delay.",
    socdUseCase: "Best for clean opposite-direction resolution while two keys are physically held.",
    reverseTap: "Reverse Tap Assist",
    reverseTapEnable: "Enable Reverse Tap (WASD)",
    reverseTapDelay: "Tap Delay (ms)",
    reverseTapDuration: "Tap Duration (ms)",
    reverseTapRandom: "Randomize delay and duration to 1..N ms",
    reverseTapDescription: "Use this for counter-strafe style release assist. Example: release W, wait for the configured delay, and then briefly tap S for the configured duration.",
    reverseTapUseCase: "Best for FPS-style stop or counter-strafe behavior after releasing a movement key.",
    scanGame: "Game Scan (ms)",
    scanOffice: "Office Scan (ms)",
    scanSaver: "Saver Scan (ms)",
    scanIdle: "Idle Scan (ms)",
    idleLowPower: "Idle To Low Scan (ms)",
    idleDeepSleep: "Idle To Deep Sleep (ms)",
    changePower: "Power Modes",
    changeReverseTap: "Reverse Tap",
    runtimeState: "Runtime State",
    runtimeMode: "Current Mode",
    runtimeScan: "Scan Interval",
    runtimeIdle: "Idle Time",
    runtimeLighting: "Lighting",
    paused: "Paused",
    active: "Active",
    close: "Close",
    helpTitle: "Help",
    helpConnectTitle: "Connect Device",
    helpConnectBody: "Use Connect to open the Config CDC port, then Read to load the current profile.",
    helpLayoutTitle: "Keyboard View",
    helpLayoutBody: "Key # shows the matrix number. Binding shows the current action only when a key has a real mapping.",
    helpTabsTitle: "Settings Tabs",
    helpTabsBody: "Use the right-side tabs to switch between keymap, lighting, and power settings without leaving the main view.",
    helpPowerTitle: "Power Modes",
    helpPowerBody: "Game, Office, and Saver define normal scan speed. Idle Scan and sleep timers are used when BLE low-power policy becomes active.",
    helpSocdTitle: "SOCD",
    helpSocdBody: "SOCD only affects final HID W/A/S/D output. When an opposite direction is pressed, the new direction can wait for the configured delay before taking over.",
    helpReverseTapTitle: "Reverse Tap Assist",
    helpReverseTapBody: "Reverse Tap Assist is different from SOCD. It can wait for a configurable delay after you release W, A, S, or D, then inject a short opposite-direction tap for the configured duration.",
    layoutSource: "LAYOUT SOURCE",
    layoutNote: "Device profile does not include visual layout data.",
    mapMode: "Key Number Map Mode",
    restoreDefaultLayout: "Restore Default Layout",
    importKleTitle: "Import Keyboard Layout Editor",
    cancel: "Cancel",
    importLayout: "Import Layout",
    localDefault: "Local Default",
    importedKle: "Imported KLE",
    importedJson: "Imported JSON",
    layoutPrefix: "LAYOUT",
    discardRead: "Discard unsaved changes and read from device?",
    confirmReboot: "Reboot keyboard now?",
  },
  zh: {
    importKle: "导入 KLE",
    importJson: "导入 JSON",
    exportJson: "导出",
    help: "帮助",
    read: "读取",
    save: "保存",
    reboot: "重启",
    connect: "连接",
    disconnect: "断开",
    disconnected: "未连接",
    connected: "已连接：ESP32-S3",
    connectFailed: "连接失败",
    profile: "配置",
    checksum: "校验",
    keys: "按键",
    layers: "层数",
    unsaved: "存在未保存修改",
    keymap: "按键",
    lighting: "灯光",
    layout: "布局",
    console: "调试信息",
    inputTest: "按键测试",
    deviceScan: "设备扫描",
    inputTestIdle: "聚焦此页面后按下键盘按键。",
    inputTestOff: "按键测试已关闭。",
    inputTestDisconnected: "请先连接键盘，再使用按键测试。",
    inputTestActive: "已按下",
    inputTestUnmapped: "未匹配的输入",
    deviceScanIdle: "设备扫描空闲。",
    deviceScanOff: "设备扫描已关闭。",
    deviceScanDisconnected: "请先连接键盘，再读取矩阵按键状态。",
    deviceScanActive: "矩阵",
    keyboardName: "键盘名称",
    changeList: "变更列表",
    changeListEmpty: "当前没有未保存的变更。",
    changeName: "键盘名称",
    changeLighting: "灯光",
    changeKeyNumber: "键号映射",
    changeBinding: "绑定",
    tabKeymap: "按键",
    tabLighting: "灯光",
    tabPower: "功耗",
    lightingPresets: "灯光配置",
    addLightingConfig: "添加配置",
    lightingEmpty: "还没有灯光配置，先添加一个。",
    lightingConfigCount: "配置数",
    lightingConfig: "配置",
    selectedLightingConfig: "当前编辑配置",
    removeLightingConfig: "删除配置",
    lightingConfigLimit: "可用的灯光配置槽位已全部用完。",
    lightingAutoCycle: "自动循环",
    lightingAutoCycleHint: "开启后，键盘上的灯效切换键会变成暂停或继续循环。",
    cycleInterval: "循环间隔（秒）",
    preset: "预设",
    activePreset: "当前预设",
    setActivePreset: "设为当前",
    selectedPreset: "编辑预设",
    lightingEditHint: "在分组常亮或逐键静态模式下，直接点击中间键盘按键即可编辑灯光。",
    lightPreviewNote: "网页预览会跟随当前选中的预设，静态模式也可以发送到设备端预览。",
    reactivePreviewHint: "按键渐灭和涟漪这类动态效果，发送预览后需要在设备上真实按下按键才会出现。",
    selectAllKeys: "全选按键",
    clearAllKeys: "全部清空",
    paintAllKeys: "全部上色",
    clearPaintedKeys: "清空已上色按键",
    staticActionSelect: "点击选择",
    staticActionPaint: "点击上色",
    keyNumberShort: "键号",
    binding: "绑定",
    keyProperties: "按键属性",
    layerBindings: "双层绑定",
    actionType: "动作类型",
    actionNone: "无",
    actionKeyboard: "键盘按键",
    actionModifier: "修饰键",
    actionConsumer: "媒体控制",
    actionFn: "Fn 层",
    actionLedToggle: "灯光开关",
    actionLedUp: "亮度增加",
    actionLedDown: "亮度降低",
    actionLedNext: "切换灯光预设",
    actionPowerModeNext: "切换功耗模式",
    keycodeSearch: "键码搜索",
    keyPicker: "键盘选键器",
    keyPickerTitle: "选择键盘按键",
    modifier: "修饰键",
    controlAction: "控制动作",
    keyNumber: "固件键号",
    clearKey: "清空",
    restore: "恢复",
    applyBinding: "应用绑定",
    currentLayer: "当前",
    editLayer: "编辑",
    baseLayer: "基础层",
    fnLayer: "Fn 层",
    effectMode: "灯效模式",
    brightness: "亮度",
    effectSpeed: "效果速度",
    rgbColor: "RGB 颜色",
    previewLighting: "预览灯光",
    saveLighting: "保存灯光到配置",
    powerModes: "功耗模式",
    powerModeDefault: "默认模式",
    powerModeCycle: "允许按键切换",
    powerModeGame: "游戏",
    powerModeOffice: "办公",
    powerModeSaver: "节能",
    socd: "SOCD",
    socdEnable: "启用 SOCD（仅 WASD）",
    socdDelay: "触发延时 (ms)",
    socdRandom: "随机延时（1~N ms）",
    socdDescription: "SOCD 用于处理两个相反方向同时成立时的输出。比如按住 A 再按 D，键盘会在设定延时后切到 D。",
    socdUseCase: "适合需要稳定处理对冲输入的场景，也就是两个方向键同时被真实按住时的判定。",
    reverseTap: "反向补键",
    reverseTapEnable: "启用反向补键（仅 WASD）",
    reverseTapDelay: "补键延迟 (ms)",
    reverseTapDuration: "补键时长 (ms)",
    reverseTapRandom: "随机延迟和时长（1~N ms）",
    reverseTapDescription: "反向补键用于松开方向键后，先等待设定延迟，再自动短按一次反方向。比如松开 W 后，等待一小段时间，再自动短按一下 S。",
    reverseTapUseCase: "适合 FPS 里的急停或 counter-strafe 一类需求，它不是 SOCD，而是松开时主动补一个反向输入。",
    scanGame: "游戏扫描 (ms)",
    scanOffice: "办公扫描 (ms)",
    scanSaver: "节能扫描 (ms)",
    scanIdle: "空闲扫描 (ms)",
    idleLowPower: "进入低速扫描 (ms)",
    idleDeepSleep: "进入深度睡眠 (ms)",
    changePower: "功耗模式",
    changeReverseTap: "反向补键",
    runtimeState: "运行状态",
    runtimeMode: "当前模式",
    runtimeScan: "扫描间隔",
    runtimeIdle: "空闲时间",
    runtimeLighting: "灯光刷新",
    paused: "已暂停",
    active: "活跃",
    close: "关闭",
    helpTitle: "帮助",
    helpConnectTitle: "连接设备",
    helpConnectBody: "先点击连接选择 Config CDC 端口，再点击读取加载键盘当前配置。",
    helpLayoutTitle: "键盘视图",
    helpLayoutBody: "Key # 表示矩阵键号，Binding 只在按键存在实际动作时才显示。",
    helpTabsTitle: "设置页签",
    helpTabsBody: "右侧页签在按键、灯光和功耗设置之间切换，不用离开主视图。",
    helpPowerTitle: "功耗模式",
    helpPowerBody: "Game、Office、Saver 对应正常扫描速度，Idle Scan 和休眠阈值在 BLE 低功耗策略中生效。",
    helpSocdTitle: "SOCD",
    helpSocdBody: "SOCD 只处理最终输出的 HID W/A/S/D。按住一个方向后再按相反方向时，新方向会按设置的延时后接管；开启随机后，实际延时会落在 1 到 N 毫秒之间。",
    helpReverseTapTitle: "反向补键",
    helpReverseTapBody: "反向补键和 SOCD 不是一回事。它会在你松开 W、A、S、D 后，先等待设定延迟，再补一个设定时长的反方向输入，适合某些游戏中的急停或反向制动。",
    layoutSource: "布局来源",
    layoutNote: "键盘 profile 当前不包含可视布局数据。",
    mapMode: "键号映射模式",
    restoreDefaultLayout: "恢复默认布局",
    importKleTitle: "导入 Keyboard Layout Editor",
    cancel: "取消",
    importLayout: "导入布局",
    localDefault: "本地默认",
    importedKle: "导入的 KLE",
    importedJson: "导入的 JSON",
    layoutPrefix: "布局",
    discardRead: "放弃未保存修改并从设备读取？",
    confirmReboot: "现在重启键盘？",
  },
};

const state = {
  serial: new YbkSerial((line) => log(line)),
  profile: createEmptyProfile(),
  pristineProfile: createEmptyProfile(),
  layout: defaultLayout(),
  pristineLayout: defaultLayout(),
  selectedKeyId: null,
  layer: 0,
  dirty: false,
  mapMode: false,
  pressedEvents: new Map(),
  pressedKeyIds: new Set(),
  devicePressedKeyIds: new Set(),
  devicePressedNumbers: [],
  scanTimer: null,
  runtimeTimer: null,
  inputTestHint: "",
  info: null,
  layoutMeta: null,
  keyboardName: defaultLayout().name,
  lang: localStorage.getItem("ybk-lang") || "zh",
  runtimeState: null,
  settingsTab: "keymap",
  lightingPresetIndex: 0,
};

const EVENT_TO_HID = new Map([
  ["KeyA", 0x04], ["KeyB", 0x05], ["KeyC", 0x06], ["KeyD", 0x07],
  ["KeyE", 0x08], ["KeyF", 0x09], ["KeyG", 0x0A], ["KeyH", 0x0B],
  ["KeyI", 0x0C], ["KeyJ", 0x0D], ["KeyK", 0x0E], ["KeyL", 0x0F],
  ["KeyM", 0x10], ["KeyN", 0x11], ["KeyO", 0x12], ["KeyP", 0x13],
  ["KeyQ", 0x14], ["KeyR", 0x15], ["KeyS", 0x16], ["KeyT", 0x17],
  ["KeyU", 0x18], ["KeyV", 0x19], ["KeyW", 0x1A], ["KeyX", 0x1B],
  ["KeyY", 0x1C], ["KeyZ", 0x1D],
  ["Digit1", 0x1E], ["Digit2", 0x1F], ["Digit3", 0x20], ["Digit4", 0x21],
  ["Digit5", 0x22], ["Digit6", 0x23], ["Digit7", 0x24], ["Digit8", 0x25],
  ["Digit9", 0x26], ["Digit0", 0x27],
  ["Enter", 0x28], ["Escape", 0x29], ["Backspace", 0x2A], ["Tab", 0x2B],
  ["Space", 0x2C], ["Minus", 0x2D], ["Equal", 0x2E],
  ["BracketLeft", 0x2F], ["BracketRight", 0x30], ["Backslash", 0x31],
  ["Semicolon", 0x33], ["Quote", 0x34], ["Backquote", 0x35],
  ["Comma", 0x36], ["Period", 0x37], ["Slash", 0x38], ["CapsLock", 0x39],
  ["F1", 0x3A], ["F2", 0x3B], ["F3", 0x3C], ["F4", 0x3D],
  ["F5", 0x3E], ["F6", 0x3F], ["F7", 0x40], ["F8", 0x41],
  ["F9", 0x42], ["F10", 0x43], ["F11", 0x44], ["F12", 0x45],
  ["PrintScreen", 0x46], ["ScrollLock", 0x47], ["Pause", 0x48],
  ["Insert", 0x49], ["Home", 0x4A], ["PageUp", 0x4B],
  ["Delete", 0x4C], ["End", 0x4D], ["PageDown", 0x4E],
  ["ArrowRight", 0x4F], ["ArrowLeft", 0x50], ["ArrowDown", 0x51], ["ArrowUp", 0x52],
  ["NumLock", 0x53], ["NumpadDivide", 0x54], ["NumpadMultiply", 0x55],
  ["NumpadSubtract", 0x56], ["NumpadAdd", 0x57], ["NumpadEnter", 0x58],
  ["Numpad1", 0x59], ["Numpad2", 0x5A], ["Numpad3", 0x5B],
  ["Numpad4", 0x5C], ["Numpad5", 0x5D], ["Numpad6", 0x5E],
  ["Numpad7", 0x5F], ["Numpad8", 0x60], ["Numpad9", 0x61], ["Numpad0", 0x62],
  ["NumpadDecimal", 0x63], ["ContextMenu", 0x65],
]);

const EVENT_TO_MODIFIER = new Map([
  ["ControlLeft", 0x01], ["ShiftLeft", 0x02], ["AltLeft", 0x04], ["MetaLeft", 0x08],
  ["ControlRight", 0x10], ["ShiftRight", 0x20], ["AltRight", 0x40], ["MetaRight", 0x80],
]);

const KEYCODE_BY_LABEL = new Map(KEYCODES.map((item) => [item.label, item.code]));

const KEY_PICKER_LAYOUT = [
  [
    ["Esc", "KC_ESC", 1],
    ["", null, 0.35, "spacer"],
    ["F1", "KC_F1", 1], ["F2", "KC_F2", 1], ["F3", "KC_F3", 1], ["F4", "KC_F4", 1],
    ["", null, 0.35, "spacer"],
    ["F5", "KC_F5", 1], ["F6", "KC_F6", 1], ["F7", "KC_F7", 1], ["F8", "KC_F8", 1],
    ["", null, 0.35, "spacer"],
    ["F9", "KC_F9", 1], ["F10", "KC_F10", 1], ["F11", "KC_F11", 1], ["F12", "KC_F12", 1],
    ["", null, 0.35, "spacer"],
    ["PrtSc", "KC_PSCREEN", 1], ["ScrLk", "KC_SCROLLLOCK", 1], ["Pause", "KC_PAUSE", 1],
  ],
  [
    ["`", "KC_GRAVE", 1], ["1", "KC_1", 1], ["2", "KC_2", 1], ["3", "KC_3", 1], ["4", "KC_4", 1],
    ["5", "KC_5", 1], ["6", "KC_6", 1], ["7", "KC_7", 1], ["8", "KC_8", 1], ["9", "KC_9", 1],
    ["0", "KC_0", 1], ["-", "KC_MINUS", 1], ["=", "KC_EQUAL", 1], ["Backspace", "KC_BSPACE", 2],
    ["", null, 0.35, "spacer"],
    ["Ins", "KC_INSERT", 1], ["Home", "KC_HOME", 1], ["PgUp", "KC_PAGEUP", 1],
    ["", null, 0.35, "spacer"],
    ["Num", "KC_NUMLOCK", 1], ["/", "KC_KP_SLASH", 1], ["*", "KC_KP_ASTERISK", 1], ["-", "KC_KP_MINUS", 1],
  ],
  [
    ["Tab", "KC_TAB", 1.5], ["Q", "KC_Q", 1], ["W", "KC_W", 1], ["E", "KC_E", 1], ["R", "KC_R", 1],
    ["T", "KC_T", 1], ["Y", "KC_Y", 1], ["U", "KC_U", 1], ["I", "KC_I", 1], ["O", "KC_O", 1],
    ["P", "KC_P", 1], ["[", "KC_LBRACKET", 1], ["]", "KC_RBRACKET", 1], ["\\", "KC_BSLASH", 1.5],
    ["", null, 0.35, "spacer"],
    ["Del", "KC_DELETE", 1], ["End", "KC_END", 1], ["PgDn", "KC_PAGEDOWN", 1],
    ["", null, 0.35, "spacer"],
    ["7", "KC_KP_7", 1], ["8", "KC_KP_8", 1], ["9", "KC_KP_9", 1], ["+", "KC_KP_PLUS", 1],
  ],
  [
    ["Caps", "KC_CAPS", 1.75], ["A", "KC_A", 1], ["S", "KC_S", 1], ["D", "KC_D", 1], ["F", "KC_F", 1],
    ["G", "KC_G", 1], ["H", "KC_H", 1], ["J", "KC_J", 1], ["K", "KC_K", 1], ["L", "KC_L", 1],
    [";", "KC_SEMICOLON", 1], ["'", "KC_QUOTE", 1], ["Enter", "KC_ENTER", 2.25],
    ["", null, 4.7, "spacer"],
    ["4", "KC_KP_4", 1], ["5", "KC_KP_5", 1], ["6", "KC_KP_6", 1], ["+", "KC_KP_PLUS", 1],
  ],
  [
    ["Shift", null, 2.25], ["Z", "KC_Z", 1], ["X", "KC_X", 1], ["C", "KC_C", 1], ["V", "KC_V", 1],
    ["B", "KC_B", 1], ["N", "KC_N", 1], ["M", "KC_M", 1], [",", "KC_COMMA", 1], [".", "KC_DOT", 1],
    ["/", "KC_SLASH", 1], ["Shift", null, 2.75],
    ["", null, 1.35, "spacer"],
    ["Up", "KC_UP", 1],
    ["", null, 0.35, "spacer"],
    ["1", "KC_KP_1", 1], ["2", "KC_KP_2", 1], ["3", "KC_KP_3", 1], ["Enter", "KC_KP_ENTER", 1],
  ],
  [
    ["Ctrl", null, 1.25], ["Win", null, 1.25], ["Alt", null, 1.25], ["Space", "KC_SPACE", 6.25],
    ["Alt", null, 1.25], ["Fn", null, 1.25], ["Menu", "KC_APPLICATION", 1.25], ["Ctrl", null, 1.25],
    ["", null, 0.35, "spacer"],
    ["Left", "KC_LEFT", 1], ["Down", "KC_DOWN", 1], ["Right", "KC_RIGHT", 1],
    ["", null, 0.35, "spacer"],
    ["0", "KC_KP_0", 2], [".", "KC_KP_DOT", 1], ["Enter", "KC_KP_ENTER", 1],
  ],
];

const ZH_KEYCAP_LABELS = new Map([
  ["Esc", "\u9000\u51fa"],
  ["PrtSc", "\u622a\u5c4f"],
  ["ScrLk", "\u6eda\u9501"],
  ["Pause", "\u6682\u505c"],
  ["Backspace", "\u9000\u683c"],
  ["Tab", "\u5236\u8868"],
  ["Caps", "\u5927\u5199"],
  ["Enter", "\u56de\u8f66"],
  ["Space", "\u7a7a\u683c"],
  ["Menu", "\u83dc\u5355"],
  ["Ins", "\u63d2\u5165"],
  ["Insert", "\u63d2\u5165"],
  ["Del", "\u5220\u9664"],
  ["Delete", "\u5220\u9664"],
  ["PgUp", "\u4e0a\u7ffb"],
  ["PgDn", "\u4e0b\u7ffb"],
  ["Up", "\u4e0a"],
  ["Down", "\u4e0b"],
  ["Left", "\u5de6"],
  ["Right", "\u53f3"],
  ["Num", "\u6570\u9501"],
]);

const ZH_SYMBOLIC_LABELS = new Map([
  ["--", "\u65e0"],
  ["ESC", "\u9000\u51fa"],
  ["ENTER", "\u56de\u8f66"],
  ["BSPACE", "\u9000\u683c"],
  ["TAB", "\u5236\u8868"],
  ["SPACE", "\u7a7a\u683c"],
  ["MINUS", "\u51cf\u53f7"],
  ["EQUAL", "\u7b49\u53f7"],
  ["LBRACKET", "\u5de6\u62ec\u53f7"],
  ["RBRACKET", "\u53f3\u62ec\u53f7"],
  ["BSLASH", "\u53cd\u659c\u6760"],
  ["SEMICOLON", "\u5206\u53f7"],
  ["QUOTE", "\u5f15\u53f7"],
  ["GRAVE", "\u53cd\u5f15\u53f7"],
  ["COMMA", "\u9017\u53f7"],
  ["DOT", "\u53e5\u70b9"],
  ["SLASH", "\u659c\u6760"],
  ["CAPS", "\u5927\u5199"],
  ["PSCREEN", "\u622a\u5c4f"],
  ["SCROLLLOCK", "\u6eda\u9501"],
  ["PAUSE", "\u6682\u505c"],
  ["INSERT", "\u63d2\u5165"],
  ["PAGEUP", "\u4e0a\u7ffb"],
  ["DELETE", "\u5220\u9664"],
  ["PAGEDOWN", "\u4e0b\u7ffb"],
  ["LEFT", "\u5de6"],
  ["RIGHT", "\u53f3"],
  ["UP", "\u4e0a"],
  ["DOWN", "\u4e0b"],
  ["NUMLOCK", "\u6570\u9501"],
  ["APPLICATION", "\u83dc\u5355"],
  ["LCTRL", "\u5de6Ctrl"],
  ["LSHIFT", "\u5de6Shift"],
  ["LALT", "\u5de6Alt"],
  ["LGUI", "\u5de6Win"],
  ["RCTRL", "\u53f3Ctrl"],
  ["RSHIFT", "\u53f3Shift"],
  ["RALT", "\u53f3Alt"],
  ["RGUI", "\u53f3Win"],
  ["MUTE", "\u9759\u97f3"],
  ["VOLUME_UP", "\u97f3\u91cf+"],
  ["VOLUME_DOWN", "\u97f3\u91cf-"],
  ["LED ON", "\u706f\u5149\u5f00\u5173"],
  ["LED +", "\u4eae\u5ea6+"],
  ["LED -", "\u4eae\u5ea6-"],
  ["LIGHT NEXT", "\u5207\u6362\u706f\u5149\u9884\u8bbe"],
  ["PWR MODE", "\u5207\u6362\u529f\u8017"],
]);

const LIGHTING_MODE_LABELS = new Map([
  [0, { en: "Windows HID", zh: "Windows \u706f\u63a7" }],
  [1, { en: "Solid", zh: "\u5e38\u4eae" }],
  [2, { en: "Sine Breath", zh: "\u6b63\u5f26\u547c\u5438" }],
  [3, { en: "Rainbow", zh: "\u5f69\u8679" }],
  [4, { en: "Wave", zh: "\u6ce2\u6d6a" }],
  [5, { en: "Group Static", zh: "\u5206\u7ec4\u5e38\u4eae" }],
  [6, { en: "Per-Key Static", zh: "\u9010\u952e\u9759\u6001" }],
  [7, { en: "Key Fade", zh: "\u6309\u952e\u6e10\u706d" }],
  [8, { en: "Ripple", zh: "\u6d9f\u6f2a" }],
]);

const POWER_MODE_LABELS = new Map([
  [0, { en: "Game", zh: "\u6e38\u620f" }],
  [1, { en: "Office", zh: "\u529e\u516c" }],
  [2, { en: "Saver", zh: "\u8282\u80fd" }],
]);

function lightingModeLabel(mode) {
  const entry = LIGHTING_MODE_LABELS.get(Number(mode));
  if (!entry) return String(mode);
  return state.lang === "zh" ? entry.zh : entry.en;
}

function powerModeLabel(mode) {
  const entry = POWER_MODE_LABELS.get(Number(mode));
  if (!entry) return String(mode);
  return state.lang === "zh" ? entry.zh : entry.en;
}

function formatIdleMs(ms) {
  if (!Number.isFinite(ms)) return "--";
  if (ms < 1000) return `${ms} ms`;
  return `${(ms / 1000).toFixed(ms >= 10000 ? 0 : 1)} s`;
}

function localizeSymbolicLabel(label) {
  if (state.lang !== "zh") return label;
  if (!label) return label;
  if (ZH_SYMBOLIC_LABELS.has(label)) return ZH_SYMBOLIC_LABELS.get(label);
  if (label.includes("+")) {
    return label.split("+").map((part) => localizeSymbolicLabel(part)).join("+");
  }
  if (label.startsWith("KC_")) {
    const translated = localizeSymbolicLabel(label.slice(3));
    return translated === label.slice(3) ? label : translated;
  }
  if (label.startsWith("KP_")) {
    const suffix = label.slice(3);
    const translated = localizeSymbolicLabel(suffix);
    return translated === suffix ? `\u5c0f\u952e${suffix}` : `\u5c0f\u952e${translated}`;
  }
  return label;
}

function displayKeyLabel(label) {
  if (state.lang !== "zh") return label;
  return ZH_KEYCAP_LABELS.get(label) || localizeSymbolicLabel(label);
}

function displayActionLabel(action) {
  return localizeSymbolicLabel(actionLabel(action || { type: ACTION_TYPES.NONE, code: 0 }));
}

function formatSelectLabel(label) {
  const localized = localizeSymbolicLabel(label);
  if (state.lang !== "zh" || localized === label) return label;
  return `${localized} (${label})`;
}

function keyPickerLabel(label, keyLabel) {
  if (state.lang !== "zh") return label;
  if (keyLabel) {
    const localized = localizeSymbolicLabel(keyLabel);
    if (localized !== keyLabel) return localized.replace(/^\u5c0f\u952e/, "\u5c0f\u952e ");
  }
  return displayKeyLabel(label);
}

function localizedLayerTag(layer) {
  if (!layer) return "";
  return state.lang === "zh" ? " Fn\u5c42" : " FN";
}

function layerDisplayName(layer) {
  if (layer === 0) return t("baseLayer");
  if (layer === 1) return t("fnLayer");
  return `LAYER ${layer}`;
}

function onOffLabel(value) {
  return state.lang === "zh" ? (value ? "\u5f00" : "\u5173") : (value ? "on" : "off");
}

function t(key) {
  return I18N[state.lang][key] || I18N.en[key] || key;
}

function applyI18n() {
  document.querySelector("#btn-import-kle").childNodes[0].textContent = t("importKle");
  document.querySelector("#btn-import-json").childNodes[0].textContent = t("importJson");
  document.querySelector("#btn-export-json").childNodes[0].textContent = t("exportJson");
  for (const node of document.querySelectorAll("[data-i18n]")) {
    const key = node.dataset.i18n;
    node.textContent = t(key);
  }
  els.keySearch.placeholder = state.lang === "zh" ? "搜索，例如 KC_ENTER" : "Search, e.g. KC_ENTER";
  els.kleInput.placeholder = state.lang === "zh" ? "粘贴 KLE Raw Data JSON" : "Paste KLE Raw Data JSON here";
  els.keyboardNameInput.placeholder = state.lang === "zh" ? "\u4f8b\u5982\uff1a\u6211\u7684\u952e\u76d8" : "For example: My Keyboard";
  els.langLabel.textContent = state.lang === "zh" ? "EN" : "CN";
  fillSelects();
  renderKeyPicker();
  updateLayoutSource();
  setConnectedUi(state.serial.connected);
}

function log(message) {
  const line = document.createElement("div");
  line.textContent = `[${new Date().toLocaleTimeString()}] ${message}`;
  els.console.appendChild(line);
  els.console.scrollTop = els.console.scrollHeight;
  while (els.console.children.length > 40) els.console.removeChild(els.console.firstChild);
}

function cloneLayoutData(layout) {
  return JSON.parse(JSON.stringify(layout));
}

function setStatus(text, connected = false) {
  els.statusText.textContent = text;
  els.statusDot.classList.toggle("connected", connected);
}

function setDirty(dirty) {
  state.dirty = dirty;
  els.warning.classList.toggle("hidden", !dirty);
  els.dirtyDot.classList.toggle("hidden", !dirty);
  renderChangeList();
}

function setSettingsTab(tab) {
  state.settingsTab = tab;
  for (const button of els.settingsTabs) {
    const active = button.dataset.settingsTab === tab;
    button.classList.toggle("active", active);
    button.setAttribute("aria-selected", active ? "true" : "false");
  }
  for (const panel of els.settingsPanels) {
    panel.classList.toggle("hidden", panel.dataset.settingsPanel !== tab);
  }
}

function sanitizeKeyboardName(name) {
  const trimmed = String(name || "").trim().replace(/\s+/g, " ");
  return (trimmed || state.layout.name || "Custom Keyboard").slice(0, 31);
}

function syncKeyboardName(name, updateInput = true) {
  state.keyboardName = sanitizeKeyboardName(name);
  state.layout.name = state.keyboardName;
  els.deviceName.textContent = state.keyboardName;
  if (updateInput && els.keyboardNameInput.value !== state.keyboardName) {
    els.keyboardNameInput.value = state.keyboardName;
  }
}

function setConnectedUi(connected) {
  els.connect.textContent = connected ? t("disconnect") : t("connect");
  els.read.disabled = !connected;
  els.save.disabled = !connected;
  els.reboot.disabled = !connected;
  els.previewLight.disabled = !connected;
  els.toggleInputTest.disabled = !connected;
  els.toggleDeviceScan.disabled = !connected;
  if (!connected) {
    stopDeviceScan();
    stopRuntimePoll();
    clearInputTest();
  } else {
    startRuntimePoll();
    if (els.toggleDeviceScan.checked) {
      startDeviceScan();
    }
  }
  setStatus(connected ? t("connected") : t("disconnected"), connected);
}

function updateLayoutSource() {
  const hash = layoutHash(state.layout);
  const source = state.layout.source === "kle"
    ? t("importedKle")
    : state.layout.source === "json"
      ? t("importedJson")
      : t("localDefault");
  els.layoutSource.textContent = `${source} / ${state.layout.layoutId || "custom"} / 0x${hash.toString(16).padStart(8, "0")}`;
  if (state.layoutMeta) {
    els.layoutNote.textContent = `Device: ${state.layoutMeta.keyboardName || "--"} / ${state.layoutMeta.layoutId || "--"} / 0x${state.layoutMeta.layoutHash.toString(16).padStart(8, "0")}`;
  } else {
    els.layoutNote.textContent = t("layoutNote");
  }
}

function layoutHash(layout) {
  const normalized = layout.keys.map((key) => ({
    label: key.label,
    keyNumber: key.keyNumber || 0,
    x: Math.round(key.x),
    y: Math.round(key.y),
    w: Math.round(key.w),
    h: Math.round(key.h),
    r: Math.round(key.r || 0),
  }));
  return fnv1a(new TextEncoder().encode(JSON.stringify(normalized)));
}

function layoutMetaPayload() {
  return encodeLayoutMeta(state.layout.layoutId || "custom", layoutHash(state.layout), state.keyboardName);
}

function selectedKey() {
  return state.layout.keys.find((key) => key.id === state.selectedKeyId) || null;
}

function conflicts() {
  const counts = new Map();
  for (const key of state.layout.keys) {
    if (!key.keyNumber) continue;
    counts.set(key.keyNumber, (counts.get(key.keyNumber) || 0) + 1);
  }
  return counts;
}

function clampLightingPresetIndex(index) {
  return Math.max(0, Math.min(PROFILE.LIGHTING_PRESET_COUNT - 1, Number(index) || 0));
}

function enabledLightingPresetIndices(profile = state.profile) {
  const lighting = getLighting(profile);
  return lighting.presets
    .map((preset, index) => (preset.enabled ? index : -1))
    .filter((index) => index >= 0);
}

function firstDisabledLightingPresetIndex(profile = state.profile) {
  const lighting = getLighting(profile);
  return lighting.presets.findIndex((preset) => !preset.enabled);
}

function makeLightingConfigDefaults(index) {
  const palette = [
    { mode: 1, brightness: 100, speed: 50, red: 255, green: 255, blue: 255, cycleIntervalSec: 8 },
    { mode: 2, brightness: 85, speed: 32, red: 90, green: 220, blue: 255, cycleIntervalSec: 10 },
    { mode: 5, brightness: 80, speed: 50, red: 255, green: 120, blue: 80, cycleIntervalSec: 8 },
    { mode: 7, brightness: 100, speed: 44, red: 120, green: 255, blue: 200, cycleIntervalSec: 6 },
    { mode: 8, brightness: 100, speed: 58, red: 120, green: 170, blue: 255, cycleIntervalSec: 6 },
    { mode: 3, brightness: 100, speed: 40, red: 255, green: 255, blue: 255, cycleIntervalSec: 8 },
  ];
  const fallback = palette[index % palette.length];
  return {
    enabled: true,
    mode: fallback.mode,
    brightness: fallback.brightness,
    speed: fallback.speed,
    red: fallback.red,
    green: fallback.green,
    blue: fallback.blue,
    cycleIntervalSec: fallback.cycleIntervalSec,
  };
}

function clampCycleInterval(value) {
  const numeric = Number(value);
  if (!Number.isFinite(numeric)) return 8;
  return Math.max(1, Math.min(120, Math.round(numeric)));
}

function ensureLightingPresetSelection(profile = state.profile) {
  const enabled = enabledLightingPresetIndices(profile);
  if (!enabled.length) {
    state.lightingPresetIndex = 0;
    return null;
  }
  if (!enabled.includes(state.lightingPresetIndex)) {
    const lighting = getLighting(profile);
    state.lightingPresetIndex = enabled.includes(lighting.activePreset) ? lighting.activePreset : enabled[0];
  }
  return state.lightingPresetIndex;
}

function selectedLightingPreset(profile = state.profile) {
  return getLightingPreset(profile, clampLightingPresetIndex(state.lightingPresetIndex));
}

function isStaticLightingMode(mode) {
  return Number(mode) === 5 || Number(mode) === 6;
}

function lightingPresetKeyCount(profile, presetIndex, mode) {
  if (Number(mode) === 5) {
    let count = 0;
    for (let keyNumber = 0; keyNumber < PROFILE.MAX_KEYS; keyNumber++) {
      if (getLightingKeyEnabled(profile, presetIndex, keyNumber)) count++;
    }
    return count;
  }
  if (Number(mode) === 6) {
    let count = 0;
    for (let keyNumber = 0; keyNumber < PROFILE.MAX_KEYS; keyNumber++) {
      if (getLightingKeyColor(profile, presetIndex, keyNumber).brightness > 0) count++;
    }
    return count;
  }
  return 0;
}

function scaledRgb(lighting) {
  const scale = Math.max(0, Math.min(100, Number(lighting.brightness || 0))) / 100;
  return {
    red: Math.round(Number(lighting.red || 0) * scale),
    green: Math.round(Number(lighting.green || 0) * scale),
    blue: Math.round(Number(lighting.blue || 0) * scale),
  };
}

function rgbCss({ red, green, blue }, alpha = 1) {
  return `rgba(${red}, ${green}, ${blue}, ${alpha})`;
}

function lightingClassForKey(key, lighting) {
  if (!lighting.enabled) return null;
  if (lighting.mode !== 6 && lighting.brightness <= 0) return null;
  const keyPressed = state.pressedKeyIds.has(key.id) || state.devicePressedKeyIds.has(key.id);
  const anyPressed = state.pressedKeyIds.size > 0 || state.devicePressedKeyIds.size > 0;
  if (lighting.mode === 5) {
    if (key.keyNumber == null || !getLightingKeyEnabled(state.profile, state.lightingPresetIndex, key.keyNumber)) {
      return null;
    }
    const color = scaledRgb(lighting);
    return {
      color: rgbCss(color, 0.9),
      modeClass: "light-static",
      duration: "0ms",
    };
  }
  if (lighting.mode === 6) {
    if (key.keyNumber == null) return null;
    const keyColor = getLightingKeyColor(state.profile, state.lightingPresetIndex, key.keyNumber);
    if (keyColor.brightness <= 0) return null;
    const color = scaledRgb(keyColor);
    return {
      color: rgbCss(color, 0.9),
      modeClass: "light-static",
      duration: "0ms",
    };
  }
  const color = scaledRgb(lighting);
  const alpha = Math.max(0.18, Math.min(0.9, Number(lighting.brightness || 0) / 115));
  let modeClass = "";
  if (lighting.mode === 2) modeClass = "light-blink";
  if (lighting.mode === 3) modeClass = "light-rainbow";
  if (lighting.mode === 4) modeClass = "light-wave";
  if (lighting.mode === 7) {
    if (!keyPressed) return null;
    modeClass = "light-blink";
  }
  if (lighting.mode === 8) {
    if (!anyPressed) return null;
    modeClass = "light-wave";
  }
  if (lighting.mode === 0) modeClass = "light-hid";
  return {
    color: rgbCss(color, keyPressed ? 0.92 : alpha),
    modeClass,
    duration: `${Math.max(360, 1500 - Number(lighting.speed || 50) * 10)}ms`,
  };
}

function handleLightingKeyEdit(key) {
  if (key.keyNumber == null) return false;
  const preset = selectedLightingPreset();
  if (preset.mode === 5) {
    const enabled = !getLightingKeyEnabled(state.profile, state.lightingPresetIndex, key.keyNumber);
    setLightingKeyEnabled(state.profile, state.lightingPresetIndex, key.keyNumber, enabled);
    if (enabled) {
      setLightingPreset(state.profile, state.lightingPresetIndex, { ...preset, enabled: true });
    }
    setDirty(true);
    renderLighting();
    renderKeyboard();
    return true;
  }
  if (preset.mode === 6) {
    const rgb = hexToRgb(els.ledColor.value);
    const brightness = Number(els.brightness.value || 0);
    setLightingKeyColor(state.profile, state.lightingPresetIndex, key.keyNumber, {
      ...rgb,
      brightness,
    });
    setLightingPreset(state.profile, state.lightingPresetIndex, { ...preset, enabled: true });
    setDirty(true);
    renderLighting();
    renderKeyboard();
    return true;
  }
  return false;
}

function staticLightingHint(preset = selectedLightingPreset()) {
  if (preset.mode === 5) return t("staticActionSelect");
  if (preset.mode === 6) return t("staticActionPaint");
  return "";
}

function setAllLightingKeys(enabled) {
  const preset = selectedLightingPreset();
  if (preset.mode !== 5) return;
  for (const key of state.layout.keys) {
    if (key.keyNumber == null) continue;
    setLightingKeyEnabled(state.profile, state.lightingPresetIndex, key.keyNumber, enabled);
  }
  if (enabled) {
    setLightingPreset(state.profile, state.lightingPresetIndex, { ...preset, enabled: true });
  }
  setDirty(true);
  renderLighting();
  renderKeyboard();
}

function clearPerKeyLighting() {
  const preset = selectedLightingPreset();
  if (preset.mode !== 6) return;
  for (const key of state.layout.keys) {
    if (key.keyNumber == null) continue;
    setLightingKeyColor(state.profile, state.lightingPresetIndex, key.keyNumber, {
      red: 0,
      green: 0,
      blue: 0,
      brightness: 0,
    });
  }
  setDirty(true);
  renderLighting();
  renderKeyboard();
}

function fillPerKeyLighting() {
  const preset = selectedLightingPreset();
  if (preset.mode !== 6) return;
  const rgb = hexToRgb(els.ledColor.value);
  const brightness = Number(els.brightness.value || 0);
  for (const key of state.layout.keys) {
    if (key.keyNumber == null) continue;
    setLightingKeyColor(state.profile, state.lightingPresetIndex, key.keyNumber, {
      ...rgb,
      brightness,
    });
  }
  setLightingPreset(state.profile, state.lightingPresetIndex, { ...preset, enabled: true });
  setDirty(true);
  renderLighting();
  renderKeyboard();
}

function addLightingConfig() {
  const freeIndex = firstDisabledLightingPresetIndex();
  if (freeIndex < 0) {
    log(`[LIGHT] ${t("lightingConfigLimit")}`);
    return false;
  }

  setLightingPreset(state.profile, freeIndex, makeLightingConfigDefaults(freeIndex));
  const enabled = enabledLightingPresetIndices();
  const active = getLighting(state.profile).activePreset;
  if (!enabled.includes(active)) {
    setLightingActivePreset(state.profile, freeIndex);
  }
  state.lightingPresetIndex = freeIndex;
  setDirty(true);
  renderLighting();
  renderKeyboard();
  return true;
}

function removeLightingConfig(index) {
  const preset = getLightingPreset(state.profile, index);
  setLightingPreset(state.profile, index, {
    ...preset,
    enabled: false,
    brightness: preset.brightness,
    speed: preset.speed,
    red: preset.red,
    green: preset.green,
    blue: preset.blue,
  });

  const enabled = enabledLightingPresetIndices();
  const lighting = getLighting(state.profile);
  if (!enabled.includes(lighting.activePreset) && enabled.length) {
    setLightingActivePreset(state.profile, enabled[0]);
  }
  ensureLightingPresetSelection();
  setDirty(true);
  renderLighting();
  renderKeyboard();
}

function eventAction(event) {
  const modifier = EVENT_TO_MODIFIER.get(event.code);
  if (modifier) {
    return { type: ACTION_TYPES.MODIFIER, code: modifier, label: event.code };
  }

  const hid = EVENT_TO_HID.get(event.code);
  if (hid) {
    return { type: ACTION_TYPES.KEY, code: hid, label: event.code };
  }

  return { type: ACTION_TYPES.NONE, code: 0, label: event.code || event.key || "--" };
}

function actionMatchesEvent(action, eventActionInfo) {
  if (eventActionInfo.type === ACTION_TYPES.KEY) {
    return action.type === ACTION_TYPES.KEY && action.code === eventActionInfo.code;
  }
  if (eventActionInfo.type === ACTION_TYPES.MODIFIER) {
    return action.type === ACTION_TYPES.MODIFIER && (action.code & eventActionInfo.code) !== 0;
  }
  return false;
}

function actionsEqual(left, right) {
  return (left?.type || 0) === (right?.type || 0) &&
    (left?.code || 0) === (right?.code || 0) &&
    (left?.flags || 0) === (right?.flags || 0);
}

function isFnDiffKey(keyNumber) {
  if (state.layer === 0 || !keyNumber) return false;
  const baseAction = getAction(state.profile, 0, keyNumber);
  const currentAction = getAction(state.profile, state.layer, keyNumber);
  return !actionsEqual(baseAction, currentAction);
}

function findKeysForEvent(eventActionInfo) {
  if (eventActionInfo.type === ACTION_TYPES.NONE) return [];
  const layers = [state.layer, ...Array.from({ length: PROFILE.LAYERS }, (_, layer) => layer).filter((layer) => layer !== state.layer)];
  const matches = [];
  const seen = new Set();
  for (const layer of layers) {
    for (const key of state.layout.keys) {
      if (!key.keyNumber || seen.has(key.id)) continue;
      const action = getAction(state.profile, layer, key.keyNumber);
      if (actionMatchesEvent(action, eventActionInfo)) {
        matches.push({ id: key.id, label: key.label, layer });
        seen.add(key.id);
      }
    }
    if (matches.length) break;
  }
  return matches;
}

function rebuildPressedKeys() {
  const pressed = new Set();
  for (const matches of state.pressedEvents.values()) {
    for (const match of matches) pressed.add(match.id);
  }
  state.pressedKeyIds = pressed;
}

function updateInputTestStatus() {
  const parts = [];
  if (!state.serial.connected) {
    parts.push(t("inputTestDisconnected"));
  } else if (els.toggleInputTest.checked) {
    const labels = [];
    for (const matches of state.pressedEvents.values()) {
      for (const match of matches) {
        labels.push(`${displayKeyLabel(match.label)}${match.layer === state.layer ? "" : localizedLayerTag(match.layer)}`);
      }
    }
    if (labels.length) {
      parts.push(`${t("inputTestActive")}: ${labels.join(", ")}`);
    } else if (state.inputTestHint) {
      parts.push(state.inputTestHint);
    } else {
      parts.push(t("inputTestIdle"));
    }
  } else {
    parts.push(t("inputTestOff"));
  }

  if (!state.serial.connected) {
    parts.push(t("deviceScanDisconnected"));
  } else if (els.toggleDeviceScan.checked) {
    parts.push(state.devicePressedNumbers.length
      ? `${t("deviceScanActive")}: ${state.devicePressedNumbers.join(", ")}`
      : t("deviceScanIdle"));
  } else {
    parts.push(t("deviceScanOff"));
  }

  els.inputTestStatus.textContent = parts.join(" | ");
}

function isEditableTarget(target) {
  return target instanceof HTMLInputElement ||
    target instanceof HTMLTextAreaElement ||
    target instanceof HTMLSelectElement ||
    target?.isContentEditable;
}

function renderKeyboard() {
  const bounds = layoutBounds(state.layout);
  els.canvas.style.width = `${bounds.width}px`;
  els.canvas.style.height = `${bounds.height}px`;
  els.canvas.innerHTML = "";
  els.layoutName.textContent = `${t("layoutPrefix").toUpperCase()}: ${state.keyboardName.toUpperCase()}`;
  updateLayoutSource();

  const keyCounts = conflicts();
  const lighting = selectedLightingPreset();
  for (const key of state.layout.keys) {
    const action = getAction(state.profile, state.layer, key.keyNumber);
    const light = lightingClassForKey(key, lighting);
    const fnDiff = isFnDiffKey(key.keyNumber);
    const button = document.createElement("button");
    button.className = "keycap";
    if (key.id === state.selectedKeyId) button.classList.add("selected");
    if (state.pressedKeyIds.has(key.id)) button.classList.add("pressed");
    if (state.devicePressedKeyIds.has(key.id)) button.classList.add("device-pressed");
    if (!key.keyNumber) button.classList.add("unmapped");
    if (key.keyNumber && keyCounts.get(key.keyNumber) > 1) button.classList.add("conflict");
    if (action.type !== ACTION_TYPES.NONE) button.classList.add("modified");
    if (fnDiff) button.classList.add("fn-diff");
    if (light) {
      button.classList.add("lit");
      if (light.modeClass) button.classList.add(light.modeClass);
      button.style.setProperty("--key-light", light.color);
      button.style.setProperty("--light-duration", light.duration);
    }
    button.style.left = `${key.x}px`;
    button.style.top = `${key.y}px`;
    button.style.width = `${key.w}px`;
    button.style.height = `${key.h}px`;
    if (key.r) {
      button.style.transformOrigin = `${key.rx || 0}px ${key.ry || 0}px`;
      button.style.transform = `rotate(${key.r}deg)`;
    }
    const staticHint = state.settingsTab === "lighting" && isStaticLightingMode(lighting.mode)
      ? ` / ${staticLightingHint(lighting)}`
      : "";
    button.title = `${state.lang === "zh" ? "\u952e" : "Key"} ${key.keyNumber || "unmapped"}: ${displayKeyLabel(key.label)}${fnDiff ? (state.lang === "zh" ? " / \u4e0e Base \u4e0d\u540c" : " / differs from Base") : ""}${staticHint}`;
    button.addEventListener("click", () => {
      if (state.settingsTab === "lighting" && isStaticLightingMode(lighting.mode) && handleLightingKeyEdit(key)) {
        return;
      }
      selectKey(key.id);
    });

    const num = document.createElement("span");
    num.className = "key-number";
    num.textContent = els.toggleKeyNumber.checked ? (key.keyNumber ?? "--") : "";

    const label = document.createElement("span");
    label.textContent = displayKeyLabel(key.label);

    const binding = document.createElement("span");
    binding.className = "key-binding";
    binding.textContent = els.toggleBinding.checked && action.type !== ACTION_TYPES.NONE
      ? displayActionLabel(action)
      : "";

    button.append(num, label, binding);
    els.canvas.appendChild(button);
  }
}

function renderLayerSwitch() {
  els.layerSwitch.innerHTML = "";
  const labels = ["BASE", "FN"];
  for (let i = 0; i < PROFILE.LAYERS; i++) {
    const button = document.createElement("button");
    button.textContent = labels[i] || `LAYER ${i}`;
    button.classList.toggle("active", state.layer === i);
    button.addEventListener("click", () => {
      state.layer = i;
      els.layerId.textContent = String(i);
      renderLayerSwitch();
      renderKeyboard();
      renderInspector();
    });
    els.layerSwitch.appendChild(button);
  }
}

function fillSelects() {
  function addOptions(select, options) {
    select.innerHTML = "";
    for (const item of options) {
      const option = document.createElement("option");
      option.value = String(item.code);
      option.textContent = `${formatSelectLabel(item.label)} 0x${item.code.toString(16).padStart(2, "0")}`;
      select.appendChild(option);
    }
  }
  addOptions(els.keySelect, KEYCODES);
  addOptions(els.consumerSelect, CONSUMERS);
  els.ledMode.innerHTML = "";
  for (const mode of [...LIGHTING_MODE_LABELS.keys()].sort((a, b) => a - b)) {
    const option = document.createElement("option");
    option.value = String(mode);
    option.textContent = lightingModeLabel(mode);
    els.ledMode.appendChild(option);
  }
  els.modifierGrid.innerHTML = "";
  for (const mod of MODIFIERS) {
    const label = document.createElement("label");
    label.innerHTML = `<input type="checkbox" value="${mod.code}"><span>${formatSelectLabel(mod.label)}</span>`;
    els.modifierGrid.appendChild(label);
  }
  els.powerModeDefault.innerHTML = "";
  for (const mode of [...POWER_MODE_LABELS.keys()].sort((a, b) => a - b)) {
    const option = document.createElement("option");
    option.value = String(mode);
    option.textContent = powerModeLabel(mode);
    els.powerModeDefault.appendChild(option);
  }
}

function renderKeyPicker() {
  els.keyPickerGrid.innerHTML = "";
  for (const row of KEY_PICKER_LAYOUT) {
    const rowEl = document.createElement("div");
    rowEl.className = "key-picker-row";
    for (const [label, keyLabel, width, kind] of row) {
      if (kind === "spacer") {
        const spacer = document.createElement("div");
        spacer.className = "key-picker-spacer";
        spacer.style.setProperty("--key-grow", String(width || 0.35));
        rowEl.appendChild(spacer);
        continue;
      }

      const button = document.createElement("button");
      button.type = "button";
      button.className = "key-picker-key";
      button.textContent = keyPickerLabel(label, keyLabel);
      button.style.setProperty("--key-grow", String(width || 1));

      const code = keyLabel ? KEYCODE_BY_LABEL.get(keyLabel) : null;
      if (code == null) {
        button.disabled = true;
      } else {
        button.addEventListener("click", () => {
          els.actionType.value = String(ACTION_TYPES.KEY);
          els.keySelect.value = String(code);
          updateActionEditor();
          applyBinding();
          els.keyPickerDialog.close();
        });
      }

      rowEl.appendChild(button);
    }
    els.keyPickerGrid.appendChild(rowEl);
  }
}

function renderInspector() {
  const key = selectedKey();
  const action = getAction(state.profile, state.layer, key?.keyNumber);
  els.keyId.textContent = key?.keyNumber ?? "--";
  els.layerId.textContent = String(state.layer);
  els.keyNumber.value = key?.keyNumber ?? "";
  els.actionType.value = String(action.type);
  els.keySelect.value = String(action.code);
  els.consumerSelect.value = String(action.code);
  for (const checkbox of els.modifierGrid.querySelectorAll("input")) {
    checkbox.checked = (action.code & Number(checkbox.value)) !== 0;
  }
  updateActionEditor();
  renderLayerBindingOverview(key);
}

function renderLayerBindingOverview(key) {
  els.layerBindingOverview.innerHTML = "";
  const baseAction = getAction(state.profile, 0, key?.keyNumber);

  for (let layer = 0; layer < PROFILE.LAYERS; layer++) {
    const action = getAction(state.profile, layer, key?.keyNumber);
    const button = document.createElement("button");
    button.type = "button";
    button.className = "layer-binding-card";
    button.classList.toggle("active", state.layer === layer);
    button.classList.toggle("different", layer > 0 && key?.keyNumber && !actionsEqual(baseAction, action));
    button.disabled = !key;
    button.innerHTML = `
      <span class="layer-binding-top">
        <span class="layer-binding-name">${layerDisplayName(layer)}</span>
        <span class="layer-binding-badge">${state.layer === layer ? t("currentLayer") : t("editLayer")}</span>
      </span>
      <span class="layer-binding-value">${key?.keyNumber ? displayActionLabel(action) : t("actionNone")}</span>
    `;
    button.addEventListener("click", () => {
      if (!key) return;
      state.layer = layer;
      renderLayerSwitch();
      renderKeyboard();
      renderInspector();
    });
    els.layerBindingOverview.appendChild(button);
  }
}

function renderLighting() {
  const lighting = getLighting(state.profile);
  const selectedIndex = ensureLightingPresetSelection();
  const hasConfigs = selectedIndex != null;
  const preset = selectedLightingPreset();
  const enabledPresets = enabledLightingPresetIndices();
  const enabledCount = enabledPresets.length;
  const selectedOrder = hasConfigs ? enabledPresets.indexOf(state.lightingPresetIndex) + 1 : 0;
  const intervalUnit = state.lang === "zh" ? "\u79d2" : "s";
  els.ledEnabled.checked = hasConfigs;
  els.addLightConfig.disabled = enabledCount >= PROFILE.LIGHTING_PRESET_COUNT;
  els.lightingAutoCycle.disabled = !hasConfigs;
  els.lightingCycleInterval.disabled = !hasConfigs;
  els.ledMode.disabled = !hasConfigs;
  els.brightness.disabled = !hasConfigs;
  els.speed.disabled = !hasConfigs;
  els.ledColor.disabled = !hasConfigs;
  els.previewLight.disabled = !state.serial.connected || !hasConfigs;
  els.activateLight.disabled = !hasConfigs;
  els.saveLight.disabled = !hasConfigs;
  els.staticSelectAll.disabled = !hasConfigs;
  els.staticClearAll.disabled = !hasConfigs;
  els.lightingAutoCycle.checked = lighting.autoCycleEnabled;
  els.lightingCycleInterval.value = String(clampCycleInterval(preset.cycleIntervalSec));
  els.ledMode.value = String(preset.mode);
  els.brightness.value = String(preset.brightness);
  els.brightnessOut.textContent = `${preset.brightness}%`;
  els.speed.value = String(preset.speed);
  els.speedOut.textContent = String(preset.speed);
  els.ledColor.value = rgbToHex(preset.red, preset.green, preset.blue);
  const staticMode = hasConfigs && isStaticLightingMode(preset.mode);
  const reactiveMode = hasConfigs && (Number(preset.mode) === 7 || Number(preset.mode) === 8);
  els.lightingStaticActions.classList.toggle("hidden", !staticMode);
  els.staticSelectAll.textContent = preset.mode === 5 ? t("selectAllKeys") : t("paintAllKeys");
  els.staticClearAll.textContent = preset.mode === 5 ? t("clearAllKeys") : t("clearPaintedKeys");
  const hints = [t("lightPreviewNote")];
  if (staticMode) hints.push(t("lightingEditHint"));
  if (reactiveMode) hints.push(t("reactivePreviewHint"));
  if (lighting.autoCycleEnabled) hints.push(t("lightingAutoCycleHint"));
  els.lightPreviewNote.textContent = hints.join(" ");
  els.lightingConfigMeta.textContent = `${t("lightingConfigCount")}: ${enabledCount} / ${PROFILE.LIGHTING_PRESET_COUNT}`;
  els.lightingSelectedMeta.textContent = hasConfigs
    ? `${t("lightingConfig")} ${selectedOrder} / ${t("cycleInterval")}: ${clampCycleInterval(preset.cycleIntervalSec)}${intervalUnit}`
    : "--";
  renderLightingPresetList(lighting);
}

function renderLightingPresetList(lighting = getLighting(state.profile)) {
  els.lightingPresetList.innerHTML = "";
  const enabled = enabledLightingPresetIndices();
  els.lightingPresetEmpty.classList.toggle("hidden", enabled.length > 0);
  enabled.forEach((index, order) => {
    const preset = lighting.presets[index];
    const keyCount = lightingPresetKeyCount(state.profile, index, preset.mode);
    const intervalUnit = state.lang === "zh" ? "\u79d2" : "s";
    const badge = lighting.activePreset === index
      ? t("activePreset")
      : state.lightingPresetIndex === index
        ? t("selectedPreset")
        : "";
    const intervalText = `${clampCycleInterval(preset.cycleIntervalSec)}${intervalUnit}`;
    const summary = isStaticLightingMode(preset.mode)
      ? `${lightingModeLabel(preset.mode)} / ${keyCount} ${state.lang === "zh" ? "\u952e" : "keys"} / ${rgbToHex(preset.red, preset.green, preset.blue).toUpperCase()} / ${intervalText}`
      : `${lightingModeLabel(preset.mode)} / ${preset.brightness}% / ${rgbToHex(preset.red, preset.green, preset.blue).toUpperCase()} / ${intervalText}`;
    const card = document.createElement("div");
    card.className = "lighting-preset-card";
    card.tabIndex = 0;
    card.setAttribute("role", "button");
    card.setAttribute("aria-pressed", state.lightingPresetIndex === index ? "true" : "false");
    card.classList.toggle("selected", state.lightingPresetIndex === index);
    card.classList.toggle("active", lighting.activePreset === index);
    card.innerHTML = `
      <span class="lighting-preset-top">
        <span class="lighting-preset-title">${t("lightingConfig")} ${order + 1}</span>
        <span class="lighting-preset-tools">
          <span class="lighting-preset-badge">${badge}</span>
          <button class="lighting-preset-remove" type="button" aria-label="${t("removeLightingConfig")}">Del</button>
        </span>
      </span>
      <span class="lighting-preset-summary">${summary}</span>
    `;
    const selectPreset = (event) => {
      if (event.target instanceof HTMLButtonElement && event.target.classList.contains("lighting-preset-remove")) return;
      state.lightingPresetIndex = index;
      renderLighting();
      renderKeyboard();
    };
    card.addEventListener("click", selectPreset);
    card.addEventListener("keydown", (event) => {
      if (event.key === "Enter" || event.key === " ") {
        event.preventDefault();
        selectPreset(event);
      }
    });
    const removeButton = card.querySelector(".lighting-preset-remove");
    removeButton.addEventListener("click", (event) => {
      event.stopPropagation();
      removeLightingConfig(index);
    });
    els.lightingPresetList.appendChild(card);
  });
}

function renderPowerSettings() {
  const power = getPowerSettings(state.profile);
  els.powerModeDefault.value = String(power.defaultMode);
  els.powerModeCycle.checked = power.allowModeCycle;
  els.socdEnabled.checked = power.socdEnabled;
  els.socdDelay.value = String(power.socdDelayMs);
  els.socdRandomize.checked = power.socdRandomize;
  els.socdDelay.disabled = !power.socdEnabled;
  els.socdRandomize.disabled = !power.socdEnabled;
  els.reverseTapEnabled.checked = power.reverseTapEnabled;
  els.reverseTapDelay.value = String(power.reverseTapDelayMs);
  els.reverseTapDuration.value = String(power.reverseTapDurationMs);
  els.reverseTapRandomize.checked = power.reverseTapRandomize;
  els.reverseTapDelay.disabled = !power.reverseTapEnabled;
  els.reverseTapDuration.disabled = !power.reverseTapEnabled;
  els.reverseTapRandomize.disabled = !power.reverseTapEnabled;
  els.scanGame.value = String(power.scanGameMs);
  els.scanOffice.value = String(power.scanOfficeMs);
  els.scanSaver.value = String(power.scanSaverMs);
  els.scanIdle.value = String(power.idleScanMs);
  els.idleLowPower.value = String(power.idleEnterLowPowerMs);
  els.idleDeepSleep.value = String(power.idleEnterDeepSleepMs);
}

function renderRuntimeState() {
  const runtime = state.runtimeState;
  els.runtimeMode.textContent = runtime ? powerModeLabel(runtime.currentMode) : "--";
  els.runtimeScan.textContent = runtime ? `${runtime.activeScanIntervalMs} ms` : "--";
  els.runtimeIdle.textContent = runtime ? formatIdleMs(runtime.idleMs) : "--";
  if (!runtime) {
    els.runtimeLighting.textContent = "--";
    return;
  }
  els.runtimeLighting.textContent = runtime.lightingPaused ? t("paused") : t("active");
}

function applyPowerSettings() {
  const readValue = (element, fallback, min, max) => {
    const value = Number(element.value);
    if (!Number.isFinite(value)) return fallback;
    return Math.max(min, Math.min(max, Math.round(value)));
  };

  setPowerSettings(state.profile, {
    defaultMode: Number(els.powerModeDefault.value || 0),
    allowModeCycle: els.powerModeCycle.checked,
    socdEnabled: els.socdEnabled.checked,
    socdDelayMs: readValue(els.socdDelay, 10, 0, 50),
    socdRandomize: els.socdRandomize.checked,
    reverseTapEnabled: els.reverseTapEnabled.checked,
    reverseTapDelayMs: readValue(els.reverseTapDelay, 0, 0, 50),
    reverseTapDurationMs: readValue(els.reverseTapDuration, 12, 0, 50),
    reverseTapRandomize: els.reverseTapRandomize.checked,
    scanGameMs: readValue(els.scanGame, 1, 1, 100),
    scanOfficeMs: readValue(els.scanOffice, 4, 1, 100),
    scanSaverMs: readValue(els.scanSaver, 8, 1, 100),
    idleScanMs: readValue(els.scanIdle, 100, 1, 100),
    idleEnterLowPowerMs: readValue(els.idleLowPower, 15000, 0, 60000),
    idleEnterDeepSleepMs: readValue(els.idleDeepSleep, 180000, 0, 3600000),
  });
  setDirty(true);
}

async function pollRuntimeState() {
  if (!state.serial.connected) return;
  const ret = await state.serial.command(COMMAND.READ_RUNTIME_STATE, new Uint8Array(), 1200);
  if (ret.status !== 0) throw new Error(ret.statusText);
  state.runtimeState = decodeRuntimeState(ret.data);
  renderRuntimeState();
}

function stopRuntimePoll() {
  if (state.runtimeTimer) {
    clearInterval(state.runtimeTimer);
    state.runtimeTimer = null;
  }
  state.runtimeState = null;
  renderRuntimeState();
}

function startRuntimePoll() {
  if (!state.serial.connected || state.runtimeTimer) return;
  state.runtimeTimer = setInterval(() => {
    pollRuntimeState().catch((error) => {
      log(`[RUNTIME] ${error.message}`);
      stopRuntimePoll();
    });
  }, 1000);
  pollRuntimeState().catch((error) => {
    log(`[RUNTIME] ${error.message}`);
    stopRuntimePoll();
  });
}

function clearInputTest() {
  state.pressedEvents.clear();
  state.inputTestHint = "";
  rebuildPressedKeys();
  renderKeyboard();
  updateInputTestStatus();
}

function updateDevicePressedKeys(keyNumbers) {
  state.devicePressedNumbers = keyNumbers;
  const ids = new Set();
  for (const key of state.layout.keys) {
    if (key.keyNumber && keyNumbers.includes(key.keyNumber)) {
      ids.add(key.id);
    }
  }
  state.devicePressedKeyIds = ids;
  renderKeyboard();
  updateInputTestStatus();
}

async function pollDeviceState() {
  if (!state.serial.connected || !els.toggleDeviceScan.checked) return;
  const ret = await state.serial.command(COMMAND.READ_KEY_STATE, new Uint8Array(), 1200);
  if (ret.status !== 0) throw new Error(ret.statusText);
  updateDevicePressedKeys(decodeKeyState(ret.data));
}

function stopDeviceScan() {
  if (state.scanTimer) {
    clearInterval(state.scanTimer);
    state.scanTimer = null;
  }
  state.devicePressedNumbers = [];
  state.devicePressedKeyIds = new Set();
  renderKeyboard();
  updateInputTestStatus();
}

function startDeviceScan() {
  if (!state.serial.connected || state.scanTimer) return;
  state.scanTimer = setInterval(() => {
    pollDeviceState().catch((error) => {
      log(`[SCAN] ${error.message}`);
      stopDeviceScan();
    });
  }, 80);
  pollDeviceState().catch((error) => {
    log(`[SCAN] ${error.message}`);
    stopDeviceScan();
  });
}

function handleInputTestKeyDown(event) {
  if (!state.serial.connected || !els.toggleInputTest.checked || isEditableTarget(event.target)) return;
  if ((event.ctrlKey || event.metaKey) && !EVENT_TO_MODIFIER.has(event.code)) return;

  const info = eventAction(event);
  if (info.type === ACTION_TYPES.NONE) {
    state.inputTestHint = `${t("inputTestUnmapped")}: ${info.label}`;
    updateInputTestStatus();
    return;
  }

  const matches = findKeysForEvent(info);
  state.pressedEvents.set(event.code, matches);
  state.inputTestHint = matches.length ? "" : `${t("inputTestUnmapped")}: ${info.label}`;
  rebuildPressedKeys();
  renderKeyboard();
  updateInputTestStatus();
  event.preventDefault();
}

function handleInputTestKeyUp(event) {
  if (!state.serial.connected || !els.toggleInputTest.checked) return;
  state.pressedEvents.delete(event.code);
  if (state.pressedEvents.size === 0) state.inputTestHint = "";
  rebuildPressedKeys();
  renderKeyboard();
  updateInputTestStatus();
}

function updateActionEditor() {
  const type = Number(els.actionType.value);
  els.keyboardEditor.classList.toggle("hidden", type !== ACTION_TYPES.KEY);
  els.modifierEditor.classList.toggle("hidden", type !== ACTION_TYPES.MODIFIER);
  els.consumerEditor.classList.toggle("hidden", type !== ACTION_TYPES.CONSUMER);
}

function selectKey(id) {
  state.selectedKeyId = id;
  renderKeyboard();
  renderInspector();
}

function applyBinding() {
  const key = selectedKey();
  if (!key) return;

  const mapped = Number(els.keyNumber.value || 0);
  key.keyNumber = mapped > 0 ? mapped : null;

  if (key.keyNumber) {
    const type = Number(els.actionType.value);
    let code = 0;
    if (type === ACTION_TYPES.KEY) code = Number(els.keySelect.value || 0);
    if (type === ACTION_TYPES.CONSUMER) code = Number(els.consumerSelect.value || 0);
    if (type === ACTION_TYPES.MODIFIER) {
      for (const checkbox of els.modifierGrid.querySelectorAll("input:checked")) {
        code |= Number(checkbox.value);
      }
    }
    setAction(state.profile, state.layer, key.keyNumber, { type, flags: 0, code });
  }

  setDirty(true);
  renderKeyboard();
  renderInspector();
  log(`[KEYMAP] Applied ${key.label} -> key ${key.keyNumber ?? "unmapped"}`);
}

function applyLightingToProfile() {
  if (ensureLightingPresetSelection() == null) return;
  const rgb = hexToRgb(els.ledColor.value);
  setLightingAutoCycleEnabled(state.profile, els.lightingAutoCycle.checked);
  setLightingPreset(state.profile, state.lightingPresetIndex, {
    enabled: true,
    mode: Number(els.ledMode.value),
    brightness: Number(els.brightness.value),
    speed: Number(els.speed.value),
    cycleIntervalSec: clampCycleInterval(els.lightingCycleInterval.value),
    ...rgb,
  });
  setDirty(true);
  renderLighting();
  updateInputTestStatus();
}

async function readDeviceProfile() {
  const info = await state.serial.command(COMMAND.GET_INFO);
  if (info.status !== 0) throw new Error(info.statusText);
  state.info = parseInfo(info.data);
  updateInfo();

  const profile = await state.serial.command(COMMAND.READ_PROFILE, new Uint8Array(), 2200);
  if (profile.status !== 0) throw new Error(profile.statusText);
  if (!validateProfile(profile.data)) throw new Error("Invalid profile received");
  state.profile = cloneProfile(profile.data);
  state.pristineProfile = cloneProfile(profile.data);
  state.lightingPresetIndex = getLighting(state.profile).activePreset;

  const layoutMeta = await state.serial.command(COMMAND.READ_LAYOUT_META, new Uint8Array(), 1600);
  if (layoutMeta.status === 0) {
    state.layoutMeta = decodeLayoutMeta(layoutMeta.data);
    applyDeviceLayoutMeta();
  } else {
    state.layoutMeta = null;
    log(`[LAYOUT] Read layout meta failed: ${layoutMeta.statusText}`);
  }

  state.pristineLayout = cloneLayoutData(state.layout);
  setDirty(false);
  renderAll();
  log("[DEVICE] Profile loaded");
}

async function saveDeviceProfile() {
  applyLightingToProfile();
  applyPowerSettings();
  const write = await state.serial.command(COMMAND.WRITE_PROFILE, state.profile, 2200);
  if (write.status !== 0) throw new Error(write.statusText);
  const commit = await state.serial.command(COMMAND.COMMIT_PROFILE, new Uint8Array(), 2200);
  if (commit.status !== 0) throw new Error(commit.statusText);

  const layoutRet = await state.serial.command(COMMAND.WRITE_LAYOUT_META, layoutMetaPayload(), 1600);
  if (layoutRet.status !== 0) throw new Error(layoutRet.statusText);
  state.layoutMeta = decodeLayoutMeta(layoutMetaPayload());

  state.pristineProfile = cloneProfile(state.profile);
  state.pristineLayout = cloneLayoutData(state.layout);
  setDirty(false);
  log("[DEVICE] Profile committed to NVS");
}

function applyDeviceLayoutMeta() {
  if (!state.layoutMeta) return;
  syncKeyboardName(state.layoutMeta.keyboardName || state.keyboardName);
  if (state.layoutMeta.layoutId === "yobboy-80" &&
      (state.layoutMeta.layoutHash === 0 || state.layoutMeta.layoutHash === layoutHash(defaultLayout()))) {
    state.layout = defaultLayout();
    syncKeyboardName(state.layoutMeta.keyboardName || state.layout.name);
    state.selectedKeyId = state.layout.keys[0]?.id || null;
    log("[LAYOUT] Device layout matched local default");
    return;
  }

  if (state.layoutMeta.layoutHash === layoutHash(state.layout)) {
    log("[LAYOUT] Current visual layout matches device metadata");
  } else {
    log(`[LAYOUT] Device layout metadata not matched locally: ${state.layoutMeta.layoutId}`);
  }
}

function parseInfo(data) {
  const view = new DataView(data.buffer, data.byteOffset, data.byteLength);
  return {
    signature: String.fromCharCode(...data.subarray(0, 4)),
    protocolVersion: view.getUint16(4, true),
    profileSize: view.getUint16(6, true),
    maxKeys: view.getUint16(8, true),
    layerCount: data[10],
    cdcIndex: data[11],
    checksum: view.getUint32(12, true),
  };
}

function updateInfo() {
  els.protocolVersion.textContent = state.info ? `Protocol v${state.info.protocolVersion}` : "Protocol --";
  els.profileVersion.textContent = `v${new DataView(state.profile.buffer).getUint16(4, true)}`;
  els.profileChecksum.textContent = `0x${checksumOfProfile(state.profile).toString(16).padStart(8, "0")}`;
  els.profileKeys.textContent = String(state.info?.maxKeys || PROFILE.MAX_KEYS);
  els.profileLayers.textContent = String(state.info?.layerCount || PROFILE.LAYERS);
}

function describeAction(action) {
  return displayActionLabel(action || { type: 0, code: 0 });
}

function currentChangeItems() {
  const items = [];
  const pristineName = sanitizeKeyboardName(state.pristineLayout?.name || "Custom Keyboard");
  if (state.keyboardName !== pristineName) {
    items.push({
      title: t("changeName"),
      detail: `${pristineName} -> ${state.keyboardName}`,
    });
  }

  const currentLighting = getLighting(state.profile);
  const baseLighting = getLighting(state.pristineProfile);
  if (JSON.stringify(lightingBytes(state.profile)) !== JSON.stringify(lightingBytes(state.pristineProfile))) {
    const fields = [];
    if (currentLighting.activePreset !== baseLighting.activePreset) {
      fields.push(`${t("activePreset")} ${baseLighting.activePreset + 1} -> ${currentLighting.activePreset + 1}`);
    }
    const changedPresets = [];
    for (let index = 0; index < PROFILE.LIGHTING_PRESET_COUNT; index++) {
      if (JSON.stringify(Array.from(lightingPresetBytes(state.profile, index))) !==
          JSON.stringify(Array.from(lightingPresetBytes(state.pristineProfile, index)))) {
        changedPresets.push(index + 1);
      }
    }
    if (changedPresets.length) {
      fields.push(`${t("lightingPresets")} P${changedPresets.join(", P")}`);
    }
    items.push({
      title: t("changeLighting"),
      detail: fields.join(" | "),
    });
  }

  const currentPower = getPowerSettings(state.profile);
  const basePower = getPowerSettings(state.pristineProfile);
  if (JSON.stringify(currentPower) !== JSON.stringify(basePower)) {
    const fields = [];
    if (currentPower.defaultMode !== basePower.defaultMode) {
      fields.push(`${t("powerModeDefault")} ${powerModeLabel(basePower.defaultMode)} -> ${powerModeLabel(currentPower.defaultMode)}`);
    }
    if (currentPower.allowModeCycle !== basePower.allowModeCycle) {
      fields.push(`${t("powerModeCycle")} ${onOffLabel(basePower.allowModeCycle)} -> ${onOffLabel(currentPower.allowModeCycle)}`);
    }
    if (currentPower.socdEnabled !== basePower.socdEnabled) {
      fields.push(`${t("socdEnable")} ${onOffLabel(basePower.socdEnabled)} -> ${onOffLabel(currentPower.socdEnabled)}`);
    }
    if (currentPower.socdDelayMs !== basePower.socdDelayMs) {
      fields.push(`${t("socdDelay")} ${basePower.socdDelayMs} -> ${currentPower.socdDelayMs}`);
    }
    if (currentPower.socdRandomize !== basePower.socdRandomize) {
      fields.push(`${t("socdRandom")} ${onOffLabel(basePower.socdRandomize)} -> ${onOffLabel(currentPower.socdRandomize)}`);
    }
    if (currentPower.reverseTapEnabled !== basePower.reverseTapEnabled) {
      fields.push(`${t("reverseTapEnable")} ${onOffLabel(basePower.reverseTapEnabled)} -> ${onOffLabel(currentPower.reverseTapEnabled)}`);
    }
    if (currentPower.reverseTapDelayMs !== basePower.reverseTapDelayMs) {
      fields.push(`${t("reverseTapDelay")} ${basePower.reverseTapDelayMs} -> ${currentPower.reverseTapDelayMs}`);
    }
    if (currentPower.reverseTapDurationMs !== basePower.reverseTapDurationMs) {
      fields.push(`${t("reverseTapDuration")} ${basePower.reverseTapDurationMs} -> ${currentPower.reverseTapDurationMs}`);
    }
    if (currentPower.reverseTapRandomize !== basePower.reverseTapRandomize) {
      fields.push(`${t("reverseTapRandom")} ${onOffLabel(basePower.reverseTapRandomize)} -> ${onOffLabel(currentPower.reverseTapRandomize)}`);
    }
    if (currentPower.scanGameMs !== basePower.scanGameMs) {
      fields.push(`${t("scanGame")} ${basePower.scanGameMs} -> ${currentPower.scanGameMs}`);
    }
    if (currentPower.scanOfficeMs !== basePower.scanOfficeMs) {
      fields.push(`${t("scanOffice")} ${basePower.scanOfficeMs} -> ${currentPower.scanOfficeMs}`);
    }
    if (currentPower.scanSaverMs !== basePower.scanSaverMs) {
      fields.push(`${t("scanSaver")} ${basePower.scanSaverMs} -> ${currentPower.scanSaverMs}`);
    }
    if (currentPower.idleScanMs !== basePower.idleScanMs) {
      fields.push(`${t("scanIdle")} ${basePower.idleScanMs} -> ${currentPower.idleScanMs}`);
    }
    if (currentPower.idleEnterLowPowerMs !== basePower.idleEnterLowPowerMs) {
      fields.push(`${t("idleLowPower")} ${basePower.idleEnterLowPowerMs} -> ${currentPower.idleEnterLowPowerMs}`);
    }
    if (currentPower.idleEnterDeepSleepMs !== basePower.idleEnterDeepSleepMs) {
      fields.push(`${t("idleDeepSleep")} ${basePower.idleEnterDeepSleepMs} -> ${currentPower.idleEnterDeepSleepMs}`);
    }
    items.push({
      title: t("changePower"),
      detail: fields.join(" | "),
    });
  }

  const pristineKeyMap = new Map((state.pristineLayout?.keys || []).map((key) => [key.id, key]));
  for (const key of state.layout.keys) {
    const baselineKey = pristineKeyMap.get(key.id);
    const currentNumber = key.keyNumber || 0;
    const baseNumber = baselineKey?.keyNumber || 0;
    if (currentNumber !== baseNumber) {
      items.push({
        title: `${t("changeKeyNumber")} ${displayKeyLabel(key.label)}`,
        detail: `${t("keyNumberShort")} ${baseNumber || "--"} -> ${currentNumber || "--"}`,
        keyId: key.id,
      });
    }

    for (let layer = 0; layer < PROFILE.LAYERS; layer++) {
      const currentAction = getAction(state.profile, layer, currentNumber);
      const baseAction = getAction(state.pristineProfile, layer, baseNumber);
      if (JSON.stringify(currentAction) !== JSON.stringify(baseAction)) {
        items.push({
          title: `${t("changeBinding")} ${displayKeyLabel(key.label)}${localizedLayerTag(layer)}`,
          detail: `${describeAction(baseAction)} -> ${describeAction(currentAction)}`,
          keyId: key.id,
          layer,
        });
      }
    }
  }

  return items;
}

function renderChangeList() {
  const items = state.dirty ? currentChangeItems() : [];
  els.changeList.innerHTML = "";
  els.changeListEmpty.classList.toggle("hidden", items.length > 0);
  for (const item of items) {
    const card = document.createElement("div");
    card.className = "change-item";
    if (item.keyId) {
      card.classList.add("locatable");
      card.tabIndex = 0;
      card.addEventListener("click", () => {
        state.layer = item.layer ?? 0;
        selectKey(item.keyId);
        renderLayerSwitch();
      });
      card.addEventListener("keydown", (event) => {
        if (event.key !== "Enter" && event.key !== " ") return;
        event.preventDefault();
        card.click();
      });
    }

    const title = document.createElement("strong");
    title.textContent = item.title;
    const detail = document.createElement("span");
    detail.textContent = item.detail;

    card.appendChild(title);
    card.appendChild(detail);
    els.changeList.appendChild(card);
  }
}

function renderAll() {
  syncKeyboardName(state.keyboardName || state.layout.name);
  updateInfo();
  setSettingsTab(state.settingsTab);
  renderLayerSwitch();
  renderKeyboard();
  renderInspector();
  renderLighting();
  renderPowerSettings();
  renderRuntimeState();
  updateInputTestStatus();
  renderChangeList();
}

function rgbToHex(red, green, blue) {
  return `#${[red, green, blue].map((v) => Number(v).toString(16).padStart(2, "0")).join("")}`;
}

function hexToRgb(hex) {
  const value = Number.parseInt(hex.replace("#", ""), 16);
  return {
    red: (value >> 16) & 0xff,
    green: (value >> 8) & 0xff,
    blue: value & 0xff,
  };
}

function bytesToBase64(bytes) {
  let binary = "";
  for (const byte of bytes) binary += String.fromCharCode(byte);
  return btoa(binary);
}

function base64ToBytes(text) {
  const binary = atob(text);
  const bytes = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i++) bytes[i] = binary.charCodeAt(i);
  return bytes;
}

function exportConfig() {
  const data = {
    version: 1,
    layout: state.layout,
    profileBase64: bytesToBase64(state.profile),
  };
  const blob = new Blob([JSON.stringify(data, null, 2)], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = "yobboy-config.json";
  a.click();
  URL.revokeObjectURL(url);
}

async function importConfig(file) {
  const text = await file.text();
  const data = JSON.parse(text);
  if (data.profileBase64) {
    const profile = base64ToBytes(data.profileBase64);
    if (!validateProfile(profile)) throw new Error("JSON profile is invalid");
    state.profile = profile;
    state.lightingPresetIndex = getLighting(state.profile).activePreset;
  }
  if (data.layout?.keys) {
    state.layout = data.layout;
    state.layout.source = state.layout.source || "json";
  }
  syncKeyboardName(state.layout.name || state.keyboardName);
  setDirty(true);
  renderAll();
  log("[IMPORT] JSON loaded");
}

function wireEvents() {
  els.connect.addEventListener("click", async () => {
    try {
      if (state.serial.connected) {
        await state.serial.disconnect();
        setConnectedUi(false);
        log("[SERIAL] Disconnected");
        return;
      }
      await state.serial.connect();
      setConnectedUi(true);
      log("[SERIAL] Connected. Select the Config CDC port.");
    } catch (error) {
      log(`[ERROR] ${error.message}`);
      setStatus(t("connectFailed"), false);
    }
  });

  els.help.addEventListener("click", () => {
    els.helpDialog.showModal();
  });

  els.addLightConfig.addEventListener("click", () => {
    if (addLightingConfig()) {
      log(`[LIGHT] ${t("addLightingConfig")}`);
    }
  });

  els.lang.addEventListener("click", () => {
    state.lang = state.lang === "zh" ? "en" : "zh";
    localStorage.setItem("ybk-lang", state.lang);
    applyI18n();
    renderAll();
  });

  els.read.addEventListener("click", async () => {
    if (state.dirty && !confirm(t("discardRead"))) return;
    try {
      await readDeviceProfile();
    } catch (error) {
      log(`[ERROR] ${error.message}`);
    }
  });

  els.save.addEventListener("click", async () => {
    try {
      await saveDeviceProfile();
    } catch (error) {
      log(`[ERROR] ${error.message}`);
    }
  });

  els.reboot.addEventListener("click", async () => {
    if (!confirm(t("confirmReboot"))) return;
    try {
      await state.serial.command(COMMAND.REBOOT);
      log("[DEVICE] Reboot command sent");
    } catch (error) {
      log(`[ERROR] ${error.message}`);
    }
  });

  els.actionType.addEventListener("change", updateActionEditor);
  for (const button of els.settingsTabs) {
    button.addEventListener("click", () => setSettingsTab(button.dataset.settingsTab));
  }
  els.applyBinding.addEventListener("click", applyBinding);
  els.clearKey.addEventListener("click", () => {
    els.actionType.value = String(ACTION_TYPES.NONE);
    applyBinding();
  });
  els.restoreKey.addEventListener("click", () => {
    const key = selectedKey();
    if (!key?.keyNumber) return;
    const action = getAction(state.pristineProfile, state.layer, key.keyNumber);
    setAction(state.profile, state.layer, key.keyNumber, action);
    setDirty(true);
    renderAll();
  });

  els.keySearch.addEventListener("input", () => {
    const q = els.keySearch.value.trim().toLowerCase();
    els.keySelect.innerHTML = "";
    for (const item of KEYCODES.filter((key) => {
      if (!q) return true;
      const localized = formatSelectLabel(key.label).toLowerCase();
      return key.label.toLowerCase().includes(q) || localized.includes(q);
    })) {
      const option = document.createElement("option");
      option.value = String(item.code);
      option.textContent = `${formatSelectLabel(item.label)} 0x${item.code.toString(16).padStart(2, "0")}`;
      els.keySelect.appendChild(option);
    }
  });
  els.openKeyPicker.addEventListener("click", () => {
    els.actionType.value = String(ACTION_TYPES.KEY);
    updateActionEditor();
    els.keyPickerDialog.showModal();
  });

  for (const el of [els.ledEnabled, els.ledMode, els.brightness, els.ledColor, els.lightingAutoCycle]) {
    el.addEventListener("input", () => {
      els.brightnessOut.textContent = `${els.brightness.value}%`;
      applyLightingToProfile();
      renderKeyboard();
    });
  }
  els.lightingCycleInterval.addEventListener("input", () => {
    els.lightingCycleInterval.value = String(clampCycleInterval(els.lightingCycleInterval.value));
    applyLightingToProfile();
    renderKeyboard();
  });
  els.speed.addEventListener("input", () => {
    els.speedOut.textContent = String(els.speed.value);
    applyLightingToProfile();
    renderKeyboard();
  });

  els.previewLight.addEventListener("click", async () => {
    try {
      applyLightingToProfile();
      if (isStaticLightingMode(selectedLightingPreset().mode)) {
        const ret = await state.serial.command(
          COMMAND.PREVIEW_LIGHTING_PRESET,
          lightingPresetBytes(state.profile, state.lightingPresetIndex),
          1600,
        );
        if (ret.status !== 0) throw new Error(ret.statusText);
        log(state.lang === "zh" ? "[LIGHT] 已发送静态灯光预览到设备" : "[LIGHT] Static lighting preview sent");
        return;
      }
      const ret = await state.serial.command(COMMAND.PREVIEW_LED, ledPreviewPayload(selectedLightingPreset()));
      if (ret.status !== 0) throw new Error(ret.statusText);
      log("[LIGHT] Preview sent");
      if (Number(selectedLightingPreset().mode) === 7 || Number(selectedLightingPreset().mode) === 8) {
        log(`[LIGHT] ${t("reactivePreviewHint")}`);
      }
    } catch (error) {
      log(`[ERROR] ${error.message}`);
    }
  });

  els.activateLight.addEventListener("click", () => {
    setLightingActivePreset(state.profile, state.lightingPresetIndex);
    setDirty(true);
    renderLighting();
    renderKeyboard();
    log(`[LIGHT] Active preset set to ${state.lightingPresetIndex + 1}`);
  });

  els.saveLight.addEventListener("click", () => {
    applyLightingToProfile();
    renderLighting();
    renderKeyboard();
  });
  els.staticSelectAll.addEventListener("click", () => {
    if (selectedLightingPreset().mode === 5) {
      setAllLightingKeys(true);
      log(state.lang === "zh" ? "[LIGHT] 已选中当前布局里的全部按键" : "[LIGHT] All keys selected");
      return;
    }
    fillPerKeyLighting();
    log(state.lang === "zh" ? "[LIGHT] 已将当前颜色应用到全部按键" : "[LIGHT] Current color applied to all keys");
  });
  els.staticClearAll.addEventListener("click", () => {
    if (selectedLightingPreset().mode === 5) {
      setAllLightingKeys(false);
      log(state.lang === "zh" ? "[LIGHT] 已清空当前分组常亮选择" : "[LIGHT] Group Static selection cleared");
      return;
    }
    clearPerKeyLighting();
    log(state.lang === "zh" ? "[LIGHT] 已清空逐键静态颜色" : "[LIGHT] Per-key lighting cleared");
  });
  for (const el of [
    els.powerModeDefault,
    els.powerModeCycle,
    els.socdEnabled,
    els.socdDelay,
    els.socdRandomize,
    els.reverseTapEnabled,
    els.reverseTapDelay,
    els.reverseTapDuration,
    els.reverseTapRandomize,
    els.scanGame,
    els.scanOffice,
    els.scanSaver,
    els.scanIdle,
    els.idleLowPower,
    els.idleDeepSleep,
  ]) {
    el.addEventListener("input", () => {
      applyPowerSettings();
      renderPowerSettings();
    });
    el.addEventListener("change", () => {
      applyPowerSettings();
      renderPowerSettings();
    });
  }
  els.toggleInputTest.addEventListener("change", () => {
    if (!els.toggleInputTest.checked) {
      clearInputTest();
    } else {
      updateInputTestStatus();
    }
  });
  els.toggleDeviceScan.addEventListener("change", () => {
    if (els.toggleDeviceScan.checked) {
      startDeviceScan();
    } else {
      stopDeviceScan();
    }
  });
  els.toggleKeyNumber.addEventListener("change", renderKeyboard);
  els.toggleBinding.addEventListener("change", renderKeyboard);

  els.importKle.addEventListener("click", () => els.kleDialog.showModal());
  els.applyKle.addEventListener("click", (event) => {
    event.preventDefault();
    try {
      state.layout = parseKle(els.kleInput.value);
      syncKeyboardName(state.layout.name);
      state.selectedKeyId = state.layout.keys[0]?.id || null;
      setDirty(true);
      renderAll();
      els.kleDialog.close();
      log("[LAYOUT] KLE layout imported");
    } catch (error) {
      log(`[ERROR] ${error.message}`);
    }
  });

  els.mapMode.addEventListener("click", () => {
    state.mapMode = !state.mapMode;
    els.mapMode.classList.toggle("primary", state.mapMode);
    els.mapMode.classList.toggle("secondary", !state.mapMode);
    log(`[LAYOUT] Key number map mode ${state.mapMode ? "enabled" : "disabled"}`);
  });
  els.keyboardNameInput.addEventListener("input", () => {
    syncKeyboardName(els.keyboardNameInput.value, false);
    setDirty(true);
    renderKeyboard();
  });
  els.defaultLayout.addEventListener("click", () => {
    state.layout = defaultLayout();
    syncKeyboardName(state.layout.name);
    state.selectedKeyId = state.layout.keys[0]?.id || null;
    setDirty(true);
    renderAll();
  });
  els.importJson.addEventListener("click", () => els.fileJson.click());
  els.fileJson.addEventListener("change", async () => {
    if (els.fileJson.files?.[0]) {
      try {
        await importConfig(els.fileJson.files[0]);
      } catch (error) {
        log(`[ERROR] ${error.message}`);
      }
    }
  });
  els.exportJson.addEventListener("click", exportConfig);
  window.addEventListener("keydown", handleInputTestKeyDown);
  window.addEventListener("keyup", handleInputTestKeyUp);
  window.addEventListener("blur", clearInputTest);
  document.addEventListener("visibilitychange", () => {
    if (document.hidden) clearInputTest();
  });
}

fillSelects();
renderKeyPicker();
state.selectedKeyId = state.layout.keys[0]?.id || null;
setSettingsTab(state.settingsTab);
wireEvents();
applyI18n();
renderAll();
setConnectedUi(false);
log("[INFO] Ready");

