---
name: Industrial Logic
colors:
  surface: '#111317'
  surface-dim: '#111317'
  surface-bright: '#37393e'
  surface-container-lowest: '#0c0e12'
  surface-container-low: '#1a1c20'
  surface-container: '#1e2024'
  surface-container-high: '#282a2e'
  surface-container-highest: '#333539'
  on-surface: '#e2e2e8'
  on-surface-variant: '#b9cacb'
  inverse-surface: '#e2e2e8'
  inverse-on-surface: '#2f3035'
  outline: '#849495'
  outline-variant: '#3b494b'
  surface-tint: '#00dbe9'
  primary: '#dbfcff'
  on-primary: '#00363a'
  primary-container: '#00f0ff'
  on-primary-container: '#006970'
  inverse-primary: '#006970'
  secondary: '#ffffff'
  on-secondary: '#223600'
  secondary-container: '#a9f900'
  on-secondary-container: '#496f00'
  tertiary: '#fff4e8'
  on-tertiary: '#412d00'
  tertiary-container: '#ffd386'
  on-tertiary-container: '#7d5800'
  error: '#ffb4ab'
  on-error: '#690005'
  error-container: '#93000a'
  on-error-container: '#ffdad6'
  primary-fixed: '#7df4ff'
  primary-fixed-dim: '#00dbe9'
  on-primary-fixed: '#002022'
  on-primary-fixed-variant: '#004f54'
  secondary-fixed: '#a9f900'
  secondary-fixed-dim: '#94db00'
  on-secondary-fixed: '#121f00'
  on-secondary-fixed-variant: '#334f00'
  tertiary-fixed: '#ffdea8'
  tertiary-fixed-dim: '#ffba20'
  on-tertiary-fixed: '#271900'
  on-tertiary-fixed-variant: '#5e4200'
  background: '#111317'
  on-background: '#e2e2e8'
  surface-variant: '#333539'
typography:
  display-lg:
    fontFamily: Space Grotesk
    fontSize: 32px
    fontWeight: '700'
    lineHeight: 40px
    letterSpacing: -0.02em
  display-sm:
    fontFamily: Space Grotesk
    fontSize: 24px
    fontWeight: '600'
    lineHeight: 32px
  body-md:
    fontFamily: Geist
    fontSize: 14px
    fontWeight: '400'
    lineHeight: 20px
  body-sm:
    fontFamily: Geist
    fontSize: 12px
    fontWeight: '400'
    lineHeight: 18px
  label-mono:
    fontFamily: JetBrains Mono
    fontSize: 11px
    fontWeight: '500'
    lineHeight: 16px
    letterSpacing: 0.05em
  label-mono-bold:
    fontFamily: JetBrains Mono
    fontSize: 11px
    fontWeight: '700'
    lineHeight: 16px
  headline-lg-mobile:
    fontFamily: Space Grotesk
    fontSize: 24px
    fontWeight: '700'
    lineHeight: 32px
rounded:
  sm: 0.125rem
  DEFAULT: 0.25rem
  md: 0.375rem
  lg: 0.5rem
  xl: 0.75rem
  full: 9999px
spacing:
  unit: 4px
  xs: 4px
  sm: 8px
  md: 16px
  lg: 24px
  xl: 40px
  gutter: 12px
  sidebar-width: 280px
---

## Brand & Style
This design system is engineered for precision and high-utility workflows. It adopts an **Industrial Minimalism** aesthetic, prioritizing information density and technical clarity over decorative flair. The personality is disciplined, authoritative, and functional—mirroring the tactile, mechanical nature of high-end hardware configuration.

The visual language is characterized by high-contrast interfaces, rigid structural grids, and a complete absence of soft gradients or organic shadows. Every element serves a functional purpose, utilizing "status-first" design where color and typography are used exclusively to communicate system states, connectivity, and configuration accuracy.

## Colors
The palette is rooted in a deep "Obsidian Slate" spectrum to minimize eye strain during long configuration sessions. 

- **Primary (Electric Blue):** Used for active interactive states, focus rings, and primary action buttons.
- **Secondary (Cyber Green):** Reserved for "Success," "Connected," or "Active Layer" indicators.
- **Tertiary (Amber):** Specifically designated for unsaved changes, warnings, or pending firmware flashes.
- **Error (Red):** High-chroma red for critical hardware conflicts or invalid key mappings.
- **Neutrals:** A range of desaturated charcoals provide structural hierarchy without introducing warmth. 

Contrast ratios are strictly maintained to ensure legibility of small-scale technical data.

## Typography
The typography system uses a tri-font approach to maximize functional hierarchy:

1. **Space Grotesk (Headers):** A geometric sans-serif with technical quirks, used for main navigation and section titles.
2. **Geist (Body):** A clean, highly legible typeface for descriptions, tooltips, and general interface text.
3. **JetBrains Mono (Labels/Data):** A monospaced font used for all technical values, keycap legends, hex codes, and status labels to ensure character alignment and a developer-centric feel.

All labels should utilize a slightly increased letter spacing and uppercase styling for a "blueprinted" look.

## Layout & Spacing
The system utilizes a **Fixed Grid** model for the main configuration workspace and a **Fluid Flex** model for sidebars and inspectors. 

- **Density:** The design favors high-density layouts. Spacing is based on a 4px baseline grid.
- **Main Workspace:** A centered fixed-width stage for the keyboard visualization.
- **Sidebars:** Fixed-width (280px) panels for "Key Selection" (Left) and "Properties" (Right).
- **Gaps:** Use a strict 12px gutter between control groups to maintain a "panel-based" industrial interface.
- **Breakpoints:** On tablet/mobile, sidebars collapse into bottom-sheets or full-screen overlays to preserve the utility of the keyboard canvas.

## Elevation & Depth
Depth is conveyed through **Tonal Layering** and **Rigid Outlines** rather than shadows. 

- **Level 0 (Background):** The deepest layer (`#0F1115`).
- **Level 1 (Panels):** Slightly lighter (`#1A1D23`) with a 1px solid border (`#30363D`).
- **Level 2 (Active Controls):** Elements like buttons or active keycaps use a higher surface brightness or a 1px primary-colored border.
- **Active State:** Instead of shadows, use a 2px offset "focus ring" in the primary accent color.
- **Inlays:** Input fields and secondary containers should appear "etched" into the UI using a darker background than their parent container.

## Shapes
In line with the industrial theme, shapes are primarily rectangular with minimal rounding. 

- **Base Radius:** 4px (Soft) for buttons and inputs to provide just enough distinction from the sharp grid.
- **Keycaps:** 6px radius to mimic the physical form of custom mechanical keys.
- **Segmented Controls:** 0px (Sharp) inner corners for joined segments, with 4px outer corners for the group.
- **Borders:** All borders are consistently 1px. Active states may increase to 2px for visual emphasis.

## Components
### Keycaps
The core component. Represented as a rounded square with a 1px border.
- **Default:** Neutral background, JetBrains Mono legend.
- **Selected:** Primary color border, subtle primary glow (low opacity).
- **Modified:** Secondary color border for keys that have been remapped.

### Segmented Controls (Layer Switching)
Used for switching between ESP32-S3 layers (0-3). These are full-width horizontal blocks with sharp corners between segments. The active segment is filled with the primary color and uses dark text for maximum contrast.

### Input Fields
Darkened "inlay" backgrounds with no top/left shadows. Focus states trigger a 1px Primary border. Technical units (e.g., "ms", "px") are appended as right-aligned JetBrains Mono labels.

### Buttons
- **Primary:** Solid background (Electric Blue) with black text.
- **Secondary:** Transparent background with 1px slate border.
- **Actionable Icons:** Monoline icons (2px stroke) with no containers unless hovered.

### Chips (Status Indicators)
Small, rectangular tags with JetBrains Mono text. Use a background tint matching the status color (e.g., 10% Green for "Connected").