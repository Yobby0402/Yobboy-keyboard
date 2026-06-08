export const PROFILE = {
  MAGIC: 0x504B4259,
  VERSION: 8,
  SIZE: 2700,
  MAX_KEYS: 80,
  LAYERS: 2,
  KEYMAP_OFFSET: 12,
  LED_OFFSET: 652,
  LIGHTING_PRESETS_OFFSET: 656,
  LIGHTING_PRESET_SIZE: 338,
  LIGHTING_PRESET_COUNT: 6,
  LIGHTING_KEY_MASK_OFFSET: 8,
  LIGHTING_KEY_MASK_BYTES: 10,
  LIGHTING_KEY_COLOR_OFFSET: 18,
  LIGHTING_KEY_COLOR_SIZE: 4,
  POWER_OFFSET: 2684,
};

export const COMMAND = {
  GET_INFO: 0x01,
  READ_PROFILE: 0x02,
  WRITE_PROFILE: 0x03,
  COMMIT_PROFILE: 0x04,
  RESET_PROFILE: 0x05,
  PREVIEW_LED: 0x06,
  REBOOT: 0x07,
  READ_LAYOUT_META: 0x08,
  WRITE_LAYOUT_META: 0x09,
  READ_KEY_STATE: 0x0A,
  READ_RUNTIME_STATE: 0x0B,
  PREVIEW_LIGHTING_PRESET: 0x0C,
  READ_LIGHTING_TOPOLOGY: 0x0D,
  WRITE_LIGHTING_TOPOLOGY: 0x0E,
  PREVIEW_LED_INDEX: 0x0F,
};

export const EVENT = {
  LOG: 0x70,
};

export const BLE = {
  SERVICE_UUID: "18f08b8e-a41d-4f5e-b0f2-1111c0de0001",
  STATUS_UUID: "18f08b8e-a41d-4f5e-b0f2-1111c0de0002",
  RX_UUID: "18f08b8e-a41d-4f5e-b0f2-1111c0de0003",
  TX_UUID: "18f08b8e-a41d-4f5e-b0f2-1111c0de0004",
  CHUNK_SIZE: 180,
};

function toUint8Array(viewLike) {
  if (viewLike instanceof Uint8Array) {
    return new Uint8Array(viewLike);
  }
  return new Uint8Array(viewLike.buffer, viewLike.byteOffset || 0, viewLike.byteLength || 0);
}

export const LAYOUT_META = {
  MAGIC: 0x4C4B4259,
  VERSION: 3,
  SIZE: 140,
  ID_LEN: 32,
  NAME_LEN: 32,
  USB_PRODUCT_NAME_LEN: 32,
  BLE_DEVICE_NAME_LEN: 32,
  V1_SIZE: 44,
  V2_SIZE: 76,
};

export const LIGHTING_TOPOLOGY = {
  MAGIC: 0x544C4259,
  VERSION: 1,
  MAX_LAMPS: 80,
  ACTIVE_LAMPS: 71,
  SIZE: 1050,
  NO_KEY: 0xff,
};

export const STATUS = {
  0x00: "OK",
  0x01: "UNKNOWN_CMD",
  0x02: "BAD_LENGTH",
  0x03: "BAD_CHECKSUM",
  0x04: "BAD_PROFILE",
  0x05: "STORAGE_ERROR",
  0x06: "INTERNAL_ERROR",
};

const CONFIG_MAGIC = 0x314B4259;

export function fnv1a(bytes) {
  let hash = 2166136261 >>> 0;
  for (const byte of bytes) {
    hash ^= byte;
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  return hash >>> 0;
}

function concatBytes(a, b) {
  const out = new Uint8Array(a.length + b.length);
  out.set(a, 0);
  out.set(b, a.length);
  return out;
}

function frame(type, seq, payload = new Uint8Array()) {
  const out = new Uint8Array(8 + payload.length + 4);
  const view = new DataView(out.buffer);
  view.setUint32(0, CONFIG_MAGIC, true);
  view.setUint8(4, type);
  view.setUint8(5, seq);
  view.setUint16(6, payload.length, true);
  out.set(payload, 8);
  view.setUint32(8 + payload.length, fnv1a(out.subarray(0, 8 + payload.length)), true);
  return out;
}

class YbkFrameTransport {
  constructor(log = () => {}) {
    this.log = log;
    this.seq = 1;
    this.rx = new Uint8Array();
    this.pending = new Map();
    this._connected = false;
  }

  get connected() {
    return this._connected;
  }

  resetSession() {
    this.seq = 1;
    this.rx = new Uint8Array();
    this.rejectPending(new Error("Transport disconnected"));
  }

  rejectPending(error) {
    for (const pending of this.pending.values()) {
      pending.reject(error);
    }
    this.pending.clear();
  }

  onBytes(value) {
    if (!value?.length) return;
    this.rx = concatBytes(this.rx, value);
    this.parseFrames();
  }

  parseFrames() {
    while (this.rx.length >= 12) {
      const view = new DataView(this.rx.buffer, this.rx.byteOffset, this.rx.byteLength);
      if (view.getUint32(0, true) !== CONFIG_MAGIC) {
        this.rx = this.rx.subarray(1);
        continue;
      }

      const type = view.getUint8(4);
      const seq = view.getUint8(5);
      const len = view.getUint16(6, true);
      const total = 8 + len + 4;
      if (this.rx.length < total) return;

      const body = this.rx.subarray(0, 8 + len);
      const expected = new DataView(this.rx.buffer, this.rx.byteOffset + 8 + len, 4).getUint32(0, true);
      const actual = fnv1a(body);
      const payload = this.rx.slice(8, 8 + len);
      this.rx = this.rx.subarray(total);

      if (expected !== actual) {
        this.log(`[PROTO] Bad response checksum: type=0x${type.toString(16)}`);
        continue;
      }

      if (type === EVENT.LOG) {
        this.log(new TextDecoder().decode(payload).trimEnd());
        continue;
      }

      const key = `${type}:${seq}`;
      const pending = this.pending.get(key);
      if (pending) {
        this.pending.delete(key);
        pending.resolve(payload);
      }
    }
  }

  async command(type, payload = new Uint8Array(), timeoutMs = 1600) {
    if (!this.connected) throw new Error("Device is not connected");
    const seq = this.seq++ & 0xff;
    const responseType = type | 0x80;
    const key = `${responseType}:${seq}`;
    const wait = new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        this.pending.delete(key);
        reject(new Error(`Command 0x${type.toString(16)} timed out`));
      }, timeoutMs);
      this.pending.set(key, {
        resolve: (payloadBytes) => {
          clearTimeout(timer);
          resolve(payloadBytes);
        },
        reject: (error) => {
          clearTimeout(timer);
          reject(error);
        },
      });
    });
    await this.writeFrame(frame(type, seq, payload));
    const response = await wait;
    const status = response[0] ?? 0xff;
    return {
      status,
      statusText: STATUS[status] || `STATUS_${status}`,
      data: response.subarray(1),
    };
  }
}

export class YbkSerial extends YbkFrameTransport {
  constructor(log = () => {}) {
    super(log);
    this.port = null;
    this.reader = null;
    this.writer = null;
  }

  async connect() {
    if (!("serial" in navigator)) {
      throw new Error("WebSerial is not available in this browser");
    }
    this.port = await navigator.serial.requestPort();
    await this.port.open({ baudRate: 115200 });
    this.reader = this.port.readable.getReader();
    this.writer = this.port.writable.getWriter();
    this._connected = true;
    this.readLoop();
  }

  async disconnect() {
    this._connected = false;
    this.resetSession();
    try {
      await this.reader?.cancel();
      this.reader?.releaseLock();
      this.writer?.releaseLock();
      await this.port?.close();
    } finally {
      this.reader = null;
      this.writer = null;
      this.port = null;
    }
  }

  async readLoop() {
    try {
      while (this.reader) {
        const { value, done } = await this.reader.read();
        if (done) break;
        this.onBytes(value);
      }
    } catch (error) {
      this.log(`[SERIAL] ${error.message}`);
    } finally {
      this._connected = false;
      this.resetSession();
    }
  }

  async writeFrame(data) {
    if (!this.writer) throw new Error("Device is not connected");
    await this.writer.write(data);
  }
}

export function decodeBleStatus(bytes) {
  if (!(bytes instanceof Uint8Array) || bytes.length < 16) {
    return null;
  }
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  return {
    protocolVersion: bytes[0],
    activeTransport: bytes[1],
    currentMode: bytes[2],
    activeScanIntervalMs: bytes[3],
    flags: bytes[4],
    profileChecksum: view.getUint32(8, true),
    idleMs: view.getUint32(12, true),
  };
}

export class YbkBluetooth extends YbkFrameTransport {
  constructor(log = () => {}) {
    super(log);
    this.device = null;
    this.server = null;
    this.service = null;
    this.statusChar = null;
    this.rxChar = null;
    this.txChar = null;
    this.lastStatus = null;
    this.onStatus = null;
    this._onTx = (event) => {
      this.onBytes(toUint8Array(event.target.value));
    };
    this._onStatus = (event) => {
      this.lastStatus = decodeBleStatus(toUint8Array(event.target.value));
      this.onStatus?.(this.lastStatus);
    };
    this._onDisconnected = () => {
      this._connected = false;
      this.resetSession();
      this.log("[BLE] Disconnected");
    };
  }

  async connect() {
    if (!("bluetooth" in navigator)) {
      throw new Error("Web Bluetooth is not available in this browser");
    }
    this.device = await navigator.bluetooth.requestDevice({
      filters: [{ services: [BLE.SERVICE_UUID] }],
      optionalServices: [BLE.SERVICE_UUID],
    });
    this.device.addEventListener("gattserverdisconnected", this._onDisconnected);
    this.server = await this.device.gatt.connect();
    this.service = await this.server.getPrimaryService(BLE.SERVICE_UUID);
    [this.statusChar, this.rxChar, this.txChar] = await Promise.all([
      this.service.getCharacteristic(BLE.STATUS_UUID),
      this.service.getCharacteristic(BLE.RX_UUID),
      this.service.getCharacteristic(BLE.TX_UUID),
    ]);

    this.txChar.addEventListener("characteristicvaluechanged", this._onTx);
    await this.txChar.startNotifications();
    this.statusChar.addEventListener("characteristicvaluechanged", this._onStatus);
    await this.statusChar.startNotifications();
    this.lastStatus = decodeBleStatus(toUint8Array(await this.statusChar.readValue()));
    this.onStatus?.(this.lastStatus);
    this._connected = true;
  }

  async disconnect() {
    this._connected = false;
    this.resetSession();
    try {
      await this.txChar?.stopNotifications?.();
      await this.statusChar?.stopNotifications?.();
    } catch {
      // ignore
    }
    this.txChar?.removeEventListener("characteristicvaluechanged", this._onTx);
    this.statusChar?.removeEventListener("characteristicvaluechanged", this._onStatus);
    if (this.device) {
      this.device.removeEventListener("gattserverdisconnected", this._onDisconnected);
    }
    if (this.server?.connected) {
      this.server.disconnect();
    }
    this.device = null;
    this.server = null;
    this.service = null;
    this.statusChar = null;
    this.rxChar = null;
    this.txChar = null;
    this.lastStatus = null;
    this.onStatus?.(null);
  }

  async writeFrame(data) {
    if (!this.rxChar) throw new Error("BLE config service is not connected");
    for (let offset = 0; offset < data.length; offset += BLE.CHUNK_SIZE) {
      const chunk = data.slice(offset, offset + BLE.CHUNK_SIZE);
      if (typeof this.rxChar.writeValueWithoutResponse === "function") {
        await this.rxChar.writeValueWithoutResponse(chunk);
      } else {
        await this.rxChar.writeValue(chunk);
      }
    }
  }
}

export function createEmptyProfile() {
  const bytes = new Uint8Array(PROFILE.SIZE);
  const view = new DataView(bytes.buffer);
  view.setUint32(0, PROFILE.MAGIC, true);
  view.setUint16(4, PROFILE.VERSION, true);
  view.setUint16(6, PROFILE.SIZE, true);
  bytes[PROFILE.LED_OFFSET] = 0;
  setLightingAutoCycleEnabled(bytes, false);
  setLightingPreset(bytes, 0, { enabled: false, mode: 1, brightness: 100, speed: 50, red: 255, green: 255, blue: 255, cycleIntervalSec: 8 });
  setLightingPreset(bytes, 1, { enabled: false, mode: 1, brightness: 100, speed: 50, red: 255, green: 255, blue: 255, cycleIntervalSec: 8 });
  setLightingPreset(bytes, 2, { enabled: false, mode: 2, brightness: 80, speed: 35, red: 80, green: 220, blue: 255, cycleIntervalSec: 10 });
  setLightingPreset(bytes, 3, { enabled: false, mode: 4, brightness: 100, speed: 35, red: 80, green: 120, blue: 255, cycleIntervalSec: 8 });
  setLightingPreset(bytes, 4, { enabled: false, mode: 3, brightness: 100, speed: 40, red: 255, green: 255, blue: 255, cycleIntervalSec: 8 });
  setLightingPreset(bytes, 5, { enabled: false, mode: 0, brightness: 100, speed: 50, red: 255, green: 255, blue: 255, cycleIntervalSec: 8 });
  bytes[PROFILE.POWER_OFFSET + 0] = 1;
  bytes[PROFILE.POWER_OFFSET + 1] = 1;
  bytes[PROFILE.POWER_OFFSET + 2] = 1;
  bytes[PROFILE.POWER_OFFSET + 3] = 4;
  bytes[PROFILE.POWER_OFFSET + 4] = 8;
  bytes[PROFILE.POWER_OFFSET + 5] = 100;
  view.setUint16(PROFILE.POWER_OFFSET + 6, 15000, true);
  view.setUint32(PROFILE.POWER_OFFSET + 8, 180000, true);
  bytes[PROFILE.POWER_OFFSET + 12] = 0;
  bytes[PROFILE.POWER_OFFSET + 13] = 10;
  bytes[PROFILE.POWER_OFFSET + 14] = 12;
  bytes[PROFILE.POWER_OFFSET + 15] = 0;
  return bytes;
}

export function cloneProfile(profile) {
  return new Uint8Array(profile);
}

export function validateProfile(profile) {
  if (!(profile instanceof Uint8Array) || profile.length !== PROFILE.SIZE) return false;
  const view = new DataView(profile.buffer, profile.byteOffset, profile.byteLength);
  const version = view.getUint16(4, true);
  return view.getUint32(0, true) === PROFILE.MAGIC &&
    (version === 5 || version === 6 || version === 7 || version === PROFILE.VERSION) &&
    view.getUint16(6, true) === PROFILE.SIZE;
}

function actionOffset(layer, keyNumber) {
  return PROFILE.KEYMAP_OFFSET + ((layer * PROFILE.MAX_KEYS + keyNumber) * 4);
}

export function getAction(profile, layer, keyNumber) {
  if (!keyNumber || keyNumber >= PROFILE.MAX_KEYS) return { type: 0, flags: 0, code: 0 };
  const off = actionOffset(layer, keyNumber);
  const view = new DataView(profile.buffer, profile.byteOffset, profile.byteLength);
  return {
    type: profile[off],
    flags: profile[off + 1],
    code: view.getUint16(off + 2, true),
  };
}

export function setAction(profile, layer, keyNumber, action) {
  if (!keyNumber || keyNumber >= PROFILE.MAX_KEYS) return;
  const off = actionOffset(layer, keyNumber);
  const view = new DataView(profile.buffer, profile.byteOffset, profile.byteLength);
  profile[off] = Number(action.type || 0);
  profile[off + 1] = Number(action.flags || 0);
  view.setUint16(off + 2, Number(action.code || 0), true);
  view.setUint32(8, 0, true);
}

export function getLighting(profile) {
  return {
    activePreset: Math.min(profile[PROFILE.LED_OFFSET] || 0, PROFILE.LIGHTING_PRESET_COUNT - 1),
    autoCycleEnabled: profile[PROFILE.LED_OFFSET + 1] !== 0,
    presets: Array.from({ length: PROFILE.LIGHTING_PRESET_COUNT }, (_, index) => getLightingPreset(profile, index)),
  };
}

export function getLightingPreset(profile, presetIndex) {
  const index = Math.max(0, Math.min(PROFILE.LIGHTING_PRESET_COUNT - 1, Number(presetIndex) || 0));
  const off = PROFILE.LIGHTING_PRESETS_OFFSET + index * PROFILE.LIGHTING_PRESET_SIZE;
  return {
    enabled: profile[off] !== 0,
    mode: profile[off + 1],
    brightness: profile[off + 2],
    speed: profile[off + 3],
    red: profile[off + 4],
    green: profile[off + 5],
    blue: profile[off + 6],
    cycleIntervalSec: Math.max(1, profile[off + 7] || 8),
  };
}

export function lightingBytes(profile) {
  return Array.from(profile.subarray(PROFILE.LED_OFFSET, PROFILE.POWER_OFFSET));
}

export function lightingPresetBytes(profile, presetIndex) {
  const index = Math.max(0, Math.min(PROFILE.LIGHTING_PRESET_COUNT - 1, Number(presetIndex) || 0));
  const off = PROFILE.LIGHTING_PRESETS_OFFSET + index * PROFILE.LIGHTING_PRESET_SIZE;
  return profile.slice(off, off + PROFILE.LIGHTING_PRESET_SIZE);
}

export function getLightingKeyEnabled(profile, presetIndex, keyNumber) {
  if (keyNumber == null || keyNumber < 0 || keyNumber >= PROFILE.MAX_KEYS) return false;
  const index = Math.max(0, Math.min(PROFILE.LIGHTING_PRESET_COUNT - 1, Number(presetIndex) || 0));
  const off = PROFILE.LIGHTING_PRESETS_OFFSET + index * PROFILE.LIGHTING_PRESET_SIZE + PROFILE.LIGHTING_KEY_MASK_OFFSET;
  const byte = profile[off + Math.floor(keyNumber / 8)];
  return (byte & (1 << (keyNumber % 8))) !== 0;
}

export function setLightingKeyEnabled(profile, presetIndex, keyNumber, enabled) {
  if (keyNumber == null || keyNumber < 0 || keyNumber >= PROFILE.MAX_KEYS) return;
  const index = Math.max(0, Math.min(PROFILE.LIGHTING_PRESET_COUNT - 1, Number(presetIndex) || 0));
  const off = PROFILE.LIGHTING_PRESETS_OFFSET + index * PROFILE.LIGHTING_PRESET_SIZE + PROFILE.LIGHTING_KEY_MASK_OFFSET;
  const byteIndex = off + Math.floor(keyNumber / 8);
  const bit = 1 << (keyNumber % 8);
  if (enabled) {
    profile[byteIndex] |= bit;
  } else {
    profile[byteIndex] &= ~bit;
  }
  new DataView(profile.buffer, profile.byteOffset, profile.byteLength).setUint32(8, 0, true);
}

export function getLightingKeyColor(profile, presetIndex, keyNumber) {
  if (keyNumber == null || keyNumber < 0 || keyNumber >= PROFILE.MAX_KEYS) {
    return { red: 0, green: 0, blue: 0, brightness: 0 };
  }
  const index = Math.max(0, Math.min(PROFILE.LIGHTING_PRESET_COUNT - 1, Number(presetIndex) || 0));
  const off = PROFILE.LIGHTING_PRESETS_OFFSET + index * PROFILE.LIGHTING_PRESET_SIZE +
    PROFILE.LIGHTING_KEY_COLOR_OFFSET + keyNumber * PROFILE.LIGHTING_KEY_COLOR_SIZE;
  return {
    red: profile[off + 0],
    green: profile[off + 1],
    blue: profile[off + 2],
    brightness: profile[off + 3],
  };
}

export function setLightingKeyColor(profile, presetIndex, keyNumber, color) {
  if (keyNumber == null || keyNumber < 0 || keyNumber >= PROFILE.MAX_KEYS) return;
  const index = Math.max(0, Math.min(PROFILE.LIGHTING_PRESET_COUNT - 1, Number(presetIndex) || 0));
  const off = PROFILE.LIGHTING_PRESETS_OFFSET + index * PROFILE.LIGHTING_PRESET_SIZE +
    PROFILE.LIGHTING_KEY_COLOR_OFFSET + keyNumber * PROFILE.LIGHTING_KEY_COLOR_SIZE;
  profile[off + 0] = Number(color.red || 0);
  profile[off + 1] = Number(color.green || 0);
  profile[off + 2] = Number(color.blue || 0);
  profile[off + 3] = Math.max(0, Math.min(100, Number(color.brightness || 0)));
  new DataView(profile.buffer, profile.byteOffset, profile.byteLength).setUint32(8, 0, true);
}

export function setLightingPreset(profile, presetIndex, lighting) {
  const index = Math.max(0, Math.min(PROFILE.LIGHTING_PRESET_COUNT - 1, Number(presetIndex) || 0));
  const off = PROFILE.LIGHTING_PRESETS_OFFSET + index * PROFILE.LIGHTING_PRESET_SIZE;
  profile[off] = lighting.enabled ? 1 : 0;
  profile[off + 1] = Number(lighting.mode || 0);
  profile[off + 2] = Math.max(0, Math.min(100, Number(lighting.brightness || 0)));
  profile[off + 3] = Math.max(1, Math.min(100, Number(lighting.speed || 50)));
  profile[off + 4] = Number(lighting.red || 0);
  profile[off + 5] = Number(lighting.green || 0);
  profile[off + 6] = Number(lighting.blue || 0);
  profile[off + 7] = Math.max(1, Math.min(120, Number(lighting.cycleIntervalSec || 8)));
  new DataView(profile.buffer, profile.byteOffset, profile.byteLength).setUint32(8, 0, true);
}

export function setLightingAutoCycleEnabled(profile, enabled) {
  profile[PROFILE.LED_OFFSET + 1] = enabled ? 1 : 0;
  new DataView(profile.buffer, profile.byteOffset, profile.byteLength).setUint32(8, 0, true);
}

export function setLightingActivePreset(profile, presetIndex) {
  profile[PROFILE.LED_OFFSET] = Math.max(0, Math.min(PROFILE.LIGHTING_PRESET_COUNT - 1, Number(presetIndex) || 0));
  new DataView(profile.buffer, profile.byteOffset, profile.byteLength).setUint32(8, 0, true);
}

export function getPowerSettings(profile) {
  const off = PROFILE.POWER_OFFSET;
  const view = new DataView(profile.buffer, profile.byteOffset, profile.byteLength);
  const version = view.getUint16(4, true);
  if (version < 8) {
    const socdDelayMs = profile[off + 13] === 0 && version < 7 ? 10 : profile[off + 13];
    return {
      defaultMode: profile[off + 0],
      allowModeCycle: profile[off + 1] !== 0,
      scanGameMs: profile[off + 2],
      scanOfficeMs: profile[off + 3],
      scanSaverMs: profile[off + 4],
      idleScanMs: profile[off + 5],
      idleEnterLowPowerMs: view.getUint16(off + 6, true),
      idleEnterDeepSleepMs: view.getUint32(off + 8, true),
      socdEnabled: profile[off + 12] !== 0,
      socdDelayMs: Math.max(0, Math.min(50, socdDelayMs)),
      socdRandomize: profile[off + 14] !== 0,
      reverseTapEnabled: false,
      reverseTapDelayMs: 0,
      reverseTapDurationMs: 12,
      reverseTapRandomize: false,
    };
  }

  const flags = profile[off + 12];
  return {
    defaultMode: profile[off + 0],
    allowModeCycle: profile[off + 1] !== 0,
    scanGameMs: profile[off + 2],
    scanOfficeMs: profile[off + 3],
    scanSaverMs: profile[off + 4],
    idleScanMs: profile[off + 5],
    idleEnterLowPowerMs: view.getUint16(off + 6, true),
    idleEnterDeepSleepMs: view.getUint32(off + 8, true),
    socdEnabled: (flags & 0x01) !== 0,
    socdDelayMs: Math.max(0, Math.min(50, profile[off + 13] || 10)),
    socdRandomize: (flags & 0x02) !== 0,
    reverseTapEnabled: (flags & 0x04) !== 0,
    reverseTapDelayMs: Math.max(0, Math.min(50, profile[off + 15] || 0)),
    reverseTapDurationMs: Math.max(0, Math.min(50, profile[off + 14] || 12)),
    reverseTapRandomize: (flags & 0x08) !== 0,
  };
}

export function setPowerSettings(profile, power) {
  const off = PROFILE.POWER_OFFSET;
  const view = new DataView(profile.buffer, profile.byteOffset, profile.byteLength);
  let flags = 0;
  if (power.socdEnabled) flags |= 0x01;
  if (power.socdRandomize) flags |= 0x02;
  if (power.reverseTapEnabled) flags |= 0x04;
  if (power.reverseTapRandomize) flags |= 0x08;
  profile[off + 0] = Number(power.defaultMode || 0);
  profile[off + 1] = power.allowModeCycle ? 1 : 0;
  profile[off + 2] = Number(power.scanGameMs || 1);
  profile[off + 3] = Number(power.scanOfficeMs || 4);
  profile[off + 4] = Number(power.scanSaverMs || 8);
  profile[off + 5] = Number(power.idleScanMs || 100);
  view.setUint16(off + 6, Number(power.idleEnterLowPowerMs || 0), true);
  view.setUint32(off + 8, Number(power.idleEnterDeepSleepMs || 0), true);
  profile[off + 12] = flags;
  profile[off + 13] = Math.max(0, Math.min(50, Number(power.socdDelayMs ?? 10)));
  profile[off + 14] = Math.max(0, Math.min(50, Number(power.reverseTapDurationMs ?? 12)));
  profile[off + 15] = Math.max(0, Math.min(50, Number(power.reverseTapDelayMs ?? 0)));
  view.setUint32(8, 0, true);
}

export function checksumOfProfile(profile) {
  return new DataView(profile.buffer, profile.byteOffset, profile.byteLength).getUint32(8, true);
}

export function ledPreviewPayload(lighting) {
  return new Uint8Array([
    Number(lighting.mode || 0),
    lighting.enabled ? 1 : 0,
    Number(lighting.brightness || 0),
    Number(lighting.speed || 50),
    Number(lighting.red || 0),
    Number(lighting.green || 0),
    Number(lighting.blue || 0),
  ]);
}

export function ledIndexPreviewPayload(ledIndex, color) {
  const bytes = new Uint8Array(6);
  const view = new DataView(bytes.buffer);
  view.setUint16(0, Math.max(0, Number(ledIndex) || 0), true);
  bytes[2] = Math.max(0, Math.min(100, Number(color?.brightness ?? 100)));
  bytes[3] = Number(color?.red || 0);
  bytes[4] = Number(color?.green || 0);
  bytes[5] = Number(color?.blue || 0);
  return bytes;
}

export function decodeKeyState(bytes) {
  if (!(bytes instanceof Uint8Array) || bytes.length < 1) {
    return [];
  }
  const count = Math.min(bytes[0], PROFILE.MAX_KEYS, bytes.length - 1);
  return Array.from(bytes.subarray(1, 1 + count));
}

export function decodeRuntimeState(bytes) {
  if (!(bytes instanceof Uint8Array) || bytes.length < 8) {
    return null;
  }
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  return {
    currentMode: bytes[0],
    idleLowScanActive: bytes[1] !== 0,
    lightingPaused: bytes[2] !== 0,
    activeScanIntervalMs: bytes[3],
    idleMs: view.getUint32(4, true),
  };
}

export function encodeLightingTopology(layout) {
  const bytes = new Uint8Array(LIGHTING_TOPOLOGY.SIZE);
  const view = new DataView(bytes.buffer);
  view.setUint32(0, LIGHTING_TOPOLOGY.MAGIC, true);
  view.setUint16(4, LIGHTING_TOPOLOGY.VERSION, true);
  view.setUint16(6, LIGHTING_TOPOLOGY.SIZE, true);
  view.setUint16(8, LIGHTING_TOPOLOGY.ACTIVE_LAMPS, true);
  view.setUint16(10, 0, true);
  bytes.fill(LIGHTING_TOPOLOGY.NO_KEY, 12, 12 + LIGHTING_TOPOLOGY.MAX_LAMPS);

  for (const key of layout?.keys || []) {
    const ledIndex = Number(key.ledIndex);
    const keyNumber = Number(key.keyNumber);
    if (!Number.isInteger(ledIndex) || ledIndex < 0 || ledIndex >= LIGHTING_TOPOLOGY.ACTIVE_LAMPS) continue;
    if (!Number.isInteger(keyNumber) || keyNumber <= 0 || keyNumber >= PROFILE.MAX_KEYS) continue;
    bytes[12 + ledIndex] = keyNumber;

    const centerX = Math.max(0, Math.round((key.x + key.w / 2) * 1000));
    const centerY = Math.max(0, Math.round((key.y + key.h / 2) * 1000));
    const posOffset = 12 + LIGHTING_TOPOLOGY.MAX_LAMPS + ledIndex * 12;
    view.setUint32(posOffset + 0, centerX, true);
    view.setUint32(posOffset + 4, centerY, true);
    view.setUint32(posOffset + 8, 5000, true);
  }

  return bytes;
}

export function decodeLightingTopology(bytes) {
  if (!(bytes instanceof Uint8Array) || bytes.length < LIGHTING_TOPOLOGY.SIZE) {
    return null;
  }
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  if (view.getUint32(0, true) !== LIGHTING_TOPOLOGY.MAGIC) {
    return null;
  }
  if (view.getUint16(4, true) !== LIGHTING_TOPOLOGY.VERSION || view.getUint16(6, true) !== LIGHTING_TOPOLOGY.SIZE) {
    return null;
  }

  const ledCount = Math.min(view.getUint16(8, true), LIGHTING_TOPOLOGY.ACTIVE_LAMPS);
  const ledToKey = Array.from(bytes.subarray(12, 12 + LIGHTING_TOPOLOGY.MAX_LAMPS));
  const positions = [];
  for (let ledIndex = 0; ledIndex < LIGHTING_TOPOLOGY.MAX_LAMPS; ledIndex++) {
    const posOffset = 12 + LIGHTING_TOPOLOGY.MAX_LAMPS + ledIndex * 12;
    positions.push({
      x: view.getUint32(posOffset + 0, true),
      y: view.getUint32(posOffset + 4, true),
      z: view.getUint32(posOffset + 8, true),
    });
  }

  return { ledCount, ledToKey, positions };
}

function encodeTextField(bytes, offset, length, value) {
  const encoded = new TextEncoder().encode(String(value || "").slice(0, length - 1));
  bytes.set(encoded, offset);
}

function decodeTextField(bytes, offset, length) {
  const slice = bytes.subarray(offset, offset + length);
  const end = slice.indexOf(0);
  return new TextDecoder().decode(end >= 0 ? slice.subarray(0, end) : slice);
}

export function encodeLayoutMeta(layoutId, layoutHash, keyboardName, usbProductName, bleDeviceName) {
  const bytes = new Uint8Array(LAYOUT_META.SIZE);
  const view = new DataView(bytes.buffer);
  view.setUint32(0, LAYOUT_META.MAGIC, true);
  view.setUint16(4, LAYOUT_META.VERSION, true);
  view.setUint16(6, LAYOUT_META.SIZE, true);
  encodeTextField(bytes, 8, LAYOUT_META.ID_LEN, layoutId || "custom");
  encodeTextField(bytes, 8 + LAYOUT_META.ID_LEN, LAYOUT_META.NAME_LEN, keyboardName || "Custom Keyboard");
  view.setUint32(8 + LAYOUT_META.ID_LEN + LAYOUT_META.NAME_LEN, layoutHash >>> 0, true);
  encodeTextField(
    bytes,
    8 + LAYOUT_META.ID_LEN + LAYOUT_META.NAME_LEN + 4,
    LAYOUT_META.USB_PRODUCT_NAME_LEN,
    usbProductName || keyboardName || "Yobboy Keyboard",
  );
  encodeTextField(
    bytes,
    8 + LAYOUT_META.ID_LEN + LAYOUT_META.NAME_LEN + 4 + LAYOUT_META.USB_PRODUCT_NAME_LEN,
    LAYOUT_META.BLE_DEVICE_NAME_LEN,
    bleDeviceName || `${keyboardName || "Yobboy Keyboard"} BLE`,
  );
  return bytes;
}

export function decodeLayoutMeta(bytes) {
  if (!(bytes instanceof Uint8Array) || bytes.length < LAYOUT_META.V1_SIZE) {
    return null;
  }
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  if (view.getUint32(0, true) !== LAYOUT_META.MAGIC) {
    return null;
  }
  const version = view.getUint16(4, true);
  const size = view.getUint16(6, true);
  const layoutId = decodeTextField(bytes, 8, LAYOUT_META.ID_LEN);

  if (version === 1 && size === LAYOUT_META.V1_SIZE) {
    return {
      layoutId,
      keyboardName: "Yobboy Keyboard",
      layoutHash: view.getUint32(8 + LAYOUT_META.ID_LEN, true),
      usbProductName: "Yobboy Keyboard",
      bleDeviceName: "Yobboy Keyboard BLE",
    };
  }

  const keyboardName = decodeTextField(bytes, 8 + LAYOUT_META.ID_LEN, LAYOUT_META.NAME_LEN) || "Yobboy Keyboard";
  if (version === 2 && size === LAYOUT_META.V2_SIZE && bytes.length >= LAYOUT_META.V2_SIZE) {
    return {
      layoutId,
      keyboardName,
      layoutHash: view.getUint32(8 + LAYOUT_META.ID_LEN + LAYOUT_META.NAME_LEN, true),
      usbProductName: keyboardName || "Yobboy Keyboard",
      bleDeviceName: `${keyboardName || "Yobboy Keyboard"} BLE`,
    };
  }

  if (version !== LAYOUT_META.VERSION || size !== LAYOUT_META.SIZE || bytes.length < LAYOUT_META.SIZE) {
    return null;
  }

  return {
    layoutId,
    keyboardName,
    layoutHash: view.getUint32(8 + LAYOUT_META.ID_LEN + LAYOUT_META.NAME_LEN, true),
    usbProductName: decodeTextField(
      bytes,
      8 + LAYOUT_META.ID_LEN + LAYOUT_META.NAME_LEN + 4,
      LAYOUT_META.USB_PRODUCT_NAME_LEN,
    ) || keyboardName || "Yobboy Keyboard",
    bleDeviceName: decodeTextField(
      bytes,
      8 + LAYOUT_META.ID_LEN + LAYOUT_META.NAME_LEN + 4 + LAYOUT_META.USB_PRODUCT_NAME_LEN,
      LAYOUT_META.BLE_DEVICE_NAME_LEN,
    ) || `${keyboardName || "Yobboy Keyboard"} BLE`,
  };
}
