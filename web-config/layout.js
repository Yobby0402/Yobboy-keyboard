const U = 48;

function makeKey(label, keyNumber, x, y, w = 1, h = 1) {
  return {
    id: `key-${keyNumber || label}-${x}-${y}`.replace(/\s+/g, "-"),
    label,
    keyNumber,
    x: x * U,
    y: y * U,
    w: w * U - 4,
    h: h * U - 4,
  };
}

export function defaultLayout() {
  const rows = [
    [
      ["Esc", 52, 0, 0], ["F1", 51, 1, 0], ["F2", 50, 2, 0], ["F3", 49, 3, 0], ["F4", 44, 4, 0],
      ["F5", 43, 5.5, 0], ["F6", 42, 6.5, 0], ["F7", 41, 7.5, 0], ["F8", 36, 8.5, 0],
      ["F9", 35, 10, 0], ["F10", 34, 11, 0], ["F11", 33, 12, 0], ["F12", 27, 13, 0],
      ["Ins", 25, 14.2, 0], ["Del", 32, 15.2, 0],
    ],
    [
      ["`", 54, 0, 1.25], ["1", 53, 1, 1.25], ["2", 56, 2, 1.25], ["3", 55, 3, 1.25],
      ["4", 46, 4, 1.25], ["5", 45, 5, 1.25], ["6", 48, 6, 1.25], ["7", 47, 7, 1.25],
      ["8", 38, 8, 1.25], ["9", 37, 9, 1.25], ["0", 40, 10, 1.25], ["-", 39, 11, 1.25],
      ["=", 28, 12, 1.25], ["Fn", 26, 13, 1.25], ["Bksp", 31, 14, 1.25, 2],
    ],
    [
      ["Tab", 64, 0, 2.5, 1.5], ["Q", 63, 1.5, 2.5], ["W", 62, 2.5, 2.5], ["E", 61, 3.5, 2.5],
      ["R", 70, 4.5, 2.5], ["T", 69, 5.5, 2.5], ["Y", 68, 6.5, 2.5], ["U", 67, 7.5, 2.5],
      ["I", 74, 8.5, 2.5], ["O", 73, 9.5, 2.5], ["P", 79, 10.5, 2.5], ["[", 78, 11.5, 2.5],
      ["]", 29, 12.5, 2.5], ["\\", 30, 13.5, 2.5, 1.5],
    ],
    [
      ["Caps", 57, 0, 3.75, 1.75], ["A", 58, 1.75, 3.75], ["S", 60, 2.75, 3.75],
      ["D", 59, 3.75, 3.75], ["F", 71, 4.75, 3.75], ["G", 72, 5.75, 3.75],
      ["H", 65, 6.75, 3.75], ["J", 66, 7.75, 3.75], ["K", 75, 8.75, 3.75],
      ["L", 76, 9.75, 3.75], [";", 77, 10.75, 3.75], ["'", 20, 11.75, 3.75],
      ["Enter", 17, 12.75, 3.75, 2.25],
    ],
    [
      ["Shift", 8, 0, 5, 2.25], ["Z", 7, 2.25, 5], ["X", 6, 3.25, 5], ["C", 5, 4.25, 5],
      ["V", 12, 5.25, 5], ["B", 11, 6.25, 5], ["N", 10, 7.25, 5], ["M", 9, 8.25, 5],
      [",", 16, 9.25, 5], [".", 15, 10.25, 5], ["/", 19, 11.25, 5], ["Up", 18, 12.25, 5],
      ["Shift", 24, 13.25, 5, 1.75],
    ],
    [
      ["Ctrl", 3, 0, 6.25, 1.25], ["Win", 2, 1.25, 6.25, 1.25], ["Alt", 1, 2.5, 6.25, 1.25],
      ["Space", 4, 3.75, 6.25, 6.25], ["Alt", 14, 10, 6.25, 1.25], ["Ctrl", 13, 11.25, 6.25, 1.25],
      ["Left", 21, 12.5, 6.25], ["Down", 22, 13.5, 6.25], ["Right", 23, 14.5, 6.25],
    ],
  ];

  return {
    name: "Yobboy 80 Key",
    layoutId: "yobboy-80",
    source: "default",
    keys: rows.flatMap((row) => row.map((key) => makeKey(...key))),
  };
}

function extractKeyNumber(label) {
  const parts = String(label).split("\n").map((part) => part.trim());
  for (const part of parts) {
    const hashMatch = part.match(/^#?(\d{1,3})$/);
    if (hashMatch) return Number(hashMatch[1]);
  }
  const bracketMatch = String(label).match(/\[(\d{1,3})\]/);
  return bracketMatch ? Number(bracketMatch[1]) : null;
}

function cleanLabel(label) {
  const parts = String(label).split("\n").map((part) => part.trim()).filter(Boolean);
  const visible = parts.find((part) => !/^#?\d{1,3}$/.test(part)) || parts[0] || "--";
  return visible.replace(/\[\d{1,3}\]/g, "").trim() || "--";
}

export function parseKle(rawText) {
  const raw = typeof rawText === "string" ? JSON.parse(rawText) : rawText;
  if (!Array.isArray(raw)) {
    throw new Error("KLE raw data must be a JSON array");
  }

  const keys = [];
  let visualId = 0;
  let cursorY = 0;

  for (const row of raw) {
    if (!Array.isArray(row)) continue;
    let cursorX = 0;
    let pending = { w: 1, h: 1, x: 0, y: 0, r: 0, rx: 0, ry: 0 };

    for (const item of row) {
      if (typeof item === "object" && item !== null) {
        pending = { ...pending, ...item };
        continue;
      }

      cursorX += Number(pending.x || 0);
      cursorY += Number(pending.y || 0);

      const keyNumber = extractKeyNumber(item);
      keys.push({
        id: `kle-${visualId++}`,
        label: cleanLabel(item),
        keyNumber,
        x: cursorX * U,
        y: cursorY * U,
        w: Number(pending.w || 1) * U - 4,
        h: Number(pending.h || 1) * U - 4,
        r: Number(pending.r || 0),
        rx: Number(pending.rx || 0) * U,
        ry: Number(pending.ry || 0) * U,
      });

      cursorX += Number(pending.w || 1);
      pending = { ...pending, w: 1, h: 1, x: 0, y: 0 };
    }

    cursorY += 1;
  }

  return {
    name: "Imported KLE Layout",
    layoutId: "kle-import",
    source: "kle",
    raw,
    keys,
  };
}

export function layoutBounds(layout) {
  const maxX = Math.max(...layout.keys.map((key) => key.x + key.w), 480);
  const maxY = Math.max(...layout.keys.map((key) => key.y + key.h), 220);
  return { width: maxX + 16, height: maxY + 16 };
}
