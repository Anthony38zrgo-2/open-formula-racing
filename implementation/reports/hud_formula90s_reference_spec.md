# Formula90s competition HUD — reference implementation specification

Target: native C++/OpenSceneGraph HUD port. This document specifies **only** the nine requested
elements and the layout/contract facts needed to reproduce them. It contains no C++ and proposes no
code.

## 0. Source, method, conventions

- Source tree: `D:\Formula90s`, branch `main-clean`, HEAD `f3efcf24`. The working tree is dirty
  (438 entries), but every file listed below is identical to HEAD; the only untracked files in the
  HUD folders are `.import`/`.uid` sidecars. The tree was read only.
- Files read in full: `game/scenes/runtime/world_hud_compositor.gd` and `.tscn`,
  `game/scenes/ui/debug_hud.tscn`, `game/scripts/hud/arcade_race_hud.gd`,
  `game/features/hud/config/hud_config.json`, `game/features/retro_hud/scripts/retro_hud_display.gd`,
  `game/features/retro_hud/scripts/competition_hud_adapter.gd`,
  `game/features/retro_hud/scenes/retro_hud_display.tscn`,
  `game/features/retro_hud/config/retro_hud.json`, all files in
  `game/features/retro_hud/assets/`, `game/scripts/hud/engine_temperature_panel.gd`,
  `game/scripts/hud/tire_status_panel.gd`, `game/scripts/hud/lap_timing_panel.gd`,
  `game/scripts/hud/driving_controls_panel.gd`, `game/scripts/track/track_minimap_controller.gd`,
  `game/scenes/ui/fuji76_77_hud_map.tres`. Supporting files also read:
  `game/features/retro_hud/scripts/retro_hud_state.gd`,
  `game/features/retro_hud/scripts/retro_hud_config.gd`, `game/scripts/hud/hud_config.gd`,
  `game/scripts/hud/arcade_speed_gauge.gd`, `game/scripts/runtime/lap_timing_controller.gd`,
  `game/scripts/track/track_map_data.gd`, `game/scripts/vehicle/vehicle_profile_configuration.gd`,
  `game/data/race_sessions/default.tres`, `game/data/tracks/fuji76_77.tres`,
  `game/data/vehicles/f1_2030_v10.tres`, the three `powertrain.json` profiles, and the HUD smoke
  tests (`game/tests/smoke_test_tire_status_panel.gd`,
  `game/tests/smoke_test_arcade_hud_scene.gd`, `game/tests/gdunit/engine_fuel_panel_test.gd`).
- Exact font and container metrics were measured with the repository's Godot 4.7.1 binary in a
  temporary project outside `D:\Formula90s` (headless; nothing was written to the source tree).
  Where a number is measured this way it is marked **[measured]**. Everything else is read directly
  from the source.
- No competition-HUD runtime screenshot was taken (no Godot session was started against the source
  project). The retro block is visually confirmed by the committed capture
  `D:\Formula90s\scratch\hud_uniform_arc\hud_17800.png` (453×340 px = the 1448×1086 design at
  ≈0.3128) and the older arcade capture `.tools/hud_current_runtime_balanced.png`.
- Coordinate systems:
  - **design px** — the `RetroHudDisplay` internal canvas, 1448×1086 (4:3).
  - **stage px** — the 16:9 `DisplayStage`; at 1920×1080 and 1280×720 stage px = window px.
  - **screen px** — stage px for all elements; the HUD is letterboxed to 16:9 inside the window.
- **Retro design→screen mapping** at 16:9:
  `screen = (stage_w − 445, stage_h − 337.75) + design × 0.2962701105`.
  At 1920×1080: origin `(1475, 742.25)`, factor `0.2962701105`
  (= `min(390/1448, 292.5/1086) × 1.1`).
- Status flags used below: **[visible]**, **[hidden]**, **[not rendered]**.

### Element status in the shipped competition HUD

| # | Element | Owning node | Status |
|---|---|---|---|
| 1 | Tachometer + speedometer | `RetroHud` (`RetroHudDisplay`) | visible |
| 2 | Fuel level/amount | `RetroHud` (fuel bar + summary) | visible; `EngineTemperaturePanel` duplicate is hidden |
| 3 | Engine water/oil temperature | `RetroHud` (OIL TEMP / WATER TEMP) | visible; `EngineTemperaturePanel` is hidden |
| 4 | Four wheel temperature sets | `TireStatusPanel` | visible (only tread ×3, carcass, disc, wear, pressure) |
| 5 | Brake disc temperatures | `TireStatusPanel` (`BRK` row) | visible |
| 6 | Minimap | `Minimap` (`TrackMinimapController`) | visible |
| 7 | Lap timing | `LapTimingPanel` | visible (LAP number, LAST, BEST; **no current time**) |
| 8 | Steering indicator | `DrivingControlsPanel` | visible when a vehicle is bound |
| 9 | Throttle/brake pedals | `DrivingControlsPanel` | visible when a vehicle is bound |

Out of scope and not specified here: `SpeedGauge` (legacy, `visible=false`), `EngineTemperaturePanel`
(hidden), `PitStopPanel`, `AidMessage`, `HandlingTuningPanel`, position/rivals/countdown (absent).

---

## 1. Composition, anchors, sizes and resize behaviour

### 1.1 Node tree (names exactly as in the scenes)

```
WorldHudCompositor (Control, full rect, mouse_filter=ignore)
├─ PsxArtController
├─ DisplayAspect (AspectRatioContainer, full rect, ratio=1.7777778,
│                 stretch_mode=2 = fit, align H/V=1 = centre, mouse_filter=ignore)
│  └─ DisplayStage (Control, container child)
│     ├─ WorldPresenter (TextureRect, full rect, expand_mode=1, mouse_filter=ignore)
│     └─ HudLayer (Control, full rect, mouse_filter=ignore)
│        └─ DebugHud (instance of scenes/ui/debug_hud.tscn, Control, full rect,
│                     mouse_filter=ignore, script = CompetitionHudAdapter)
│           ├─ HandlingTuningPanel (Control, full rect; inner LiveTuningPanel hidden)
│           ├─ LapAndMapBackground (ColorRect)
│           ├─ Minimap (TrackMinimapController)
│           ├─ SpeedGauge (ArcadeSpeedGauge, visible=false)
│           ├─ RetroHud (instance of features/retro_hud/scenes/retro_hud_display.tscn)
│           ├─ AidMessage (Label, top-right, hidden; out of scope)
│           ├─ TireStatusPanel (runtime PanelContainer)
│           ├─ EngineTemperaturePanel (runtime PanelContainer, visible=false)
│           ├─ LapTimingPanel (runtime PanelContainer)
│           ├─ PitStopPanel (runtime PanelContainer; out of scope)
│           └─ DrivingControlsPanel (runtime Control)
└─ WorldViewport (SubViewport, size 320×240 initially, msaa_3d=1, update_mode=4)
```

Draw order is child order, so the background block is under the minimap, the runtime panels are
above the retro HUD, and the driving panel is last.

### 1.2 Scene anchors and offsets

| Node | Anchors | Offsets (L,T,R,B) | Size | Scale |
|---|---|---|---|---|
| `WorldHudCompositor` | preset full rect (0,0,1,1) | 0 | window | 1 |
| `DisplayAspect` | full rect | 0 | window | 1 |
| `DisplayStage` | container | — | 16:9 fit | 1 |
| `WorldPresenter` | full rect | 0 | stage | 1 |
| `HudLayer` | full rect (re-applied on resize) | 0 | stage | 1 |
| `DebugHud` | full rect (re-applied on resize) | 0 | stage | 1 |
| `LapAndMapBackground` | top-left | 14, 10, 366, 570 (overridden at runtime) | 352×560 scene → 285.36×480.816 runtime | 1 |
| `Minimap` | top-left | 54, 312, 162, 440 | 108×128 | 2.25 |
| `SpeedGauge` | bottom-right (preset 3) | −194, −126, −18, −16 | 176×110 | 1, `visible=false` |
| `RetroHud` | bottom-right (preset 3) | −445, −337.75, −55, −45.25 | 390×292.5 | 1.1 |
| `AidMessage` | top-right | −300, 18, −18, 50 | 282×32 | 1, hidden |
| `TireStatusPanel` | none (runtime, global_position) | — | content 402×297 | 1.188 |
| `EngineTemperaturePanel` | none (runtime) | — | 390×168 | 1.188, hidden |
| `LapTimingPanel` | none (runtime) | — | content 220×132 | 1.188 |
| `PitStopPanel` | none (runtime) | — | 400×174 | 1.188, out of scope |
| `DrivingControlsPanel` | none (runtime) | — | 396×116 | `clamp(stage_w/1280, 0.65, 1)` |

`RetroHud` keeps `custom_minimum_size = (390, 292.5)` from its scene and is scaled by 1.1, so its
visual rectangle is **429×321.75**, i.e. 16 px from the right and 16 px from the bottom at every
resolution.

### 1.3 Resize path

`WorldHudCompositor._ready()` connects `DisplayStage.resized` to `_sync_native_viewport_size()`;
that function re-applies full-rect anchors/offsets to `HudLayer` and `DebugHud` and resizes the
`WorldViewport` to the stage (PSX is disabled by default: `psx_enabled=false`, so the presenter has
no material and uses nearest filtering). There is no global HUD scale: all elements except the
driving panel are fixed-size and anchored. The 16:9 stage itself is produced by the
`AspectRatioContainer`, so on non-16:9 windows the HUD lives inside the centred 16:9 rectangle and
black bars appear outside it.

Two coordinate quirks matter for a faithful port:

1. `_position_vehicle_status_panels()` and `_position_pit_stop_panel()` clamp against
   `get_viewport_rect().size` (the **window**), not the 16:9 stage. On 16:9 they are identical; on
   ultrawide/tall windows the clamp bounds do not match the visible stage.
2. `Control.get_global_rect()` returns the **unscaled** rect. The tire panel is centred on the
   retro control's unscaled centre (`retro.left + 195`), not on the scaled visual centre
   (`retro.left + 214.5`). At 1920×1080 this puts the tire panel centre at x=1670 while the retro
   visual centre is 1689.5, i.e. the tire panel sits 19.5 px to the left of the visual centre. The
   same unscaled rect is used for the vertical clamp's "bottom" reference (1034.75 instead of the
   visual 1064).

### 1.4 Placement formulas (stage px)

Let `W = stage width`, `H = stage height`.

**RetroHud**
`left = W − 445`, `top = H − 337.75`, `size = 390×292.5`, `scale = 1.1`,
visual rect `(left, top, 429, 321.75)`.

**TireStatusPanel** (engine panel hidden)
`visual_w = 402 × 1.188 = 477.576`, `visual_h = 297 × 1.188 = 352.836`.
`centre_x = (W − 445) + 195 = W − 250`
`left = centre_x − 238.788 = W − 488.788`
`top = (H − 337.75) − 352.836 − 10 = H − 700.586`
Vertical clamp (applied to the whole group):
`group_top = clamp(top, 6, max(6, window_h − 655.336 − 6))`, then both panels are shifted by
`group_top − top`. With the engine panel hidden the clamp is inactive for `H ≥ 706.586`; the tire
panel overlaps the retro HUD below `H ≈ 696.6`. 720p is the smallest 16:9 height with a clean
10 px gap.

**Lap + map block**
`block_pos = (14, 10)`, `padding = 12`, `gap = 12`.
`lap_visual = 220 × 1.188 = 261.36` wide, `132 × 1.188 = 156.816` high.
`map_visual = 108 × 2.25 = 243` wide, `128 × 2.25 = 288` high.
`content_w = max(261.36, 243) = 261.36`.
`lap_pos = (14 + 12, 10 + 12) = (26, 22)`.
`map_pos = (26 + (261.36 − 243)/2, 22 + 156.816 + 12) = (35.18, 190.816)`.
`block_size = (261.36 + 24, 156.816 + 12 + 288 + 24) = (285.36, 480.816)`.
The block is visible iff the lap panel or the minimap is visible.

**DrivingControlsPanel**
`s = clamp(W / 1280, 0.65, 1.0)`, `pos = ((W − 396·s)/2, H − 116·s − 18)`.
At `W ≥ 1280` `s = 1`. At `W = 1280` the panel is `(442, 586)–(838, 702)`; its right edge is
3 px past the retro HUD's left edge (835), so the original already has a 3 px overlap at 720p. At
1024×576 (4:3 stage) the overlap is ≈91 px horizontally and the tire panel overlaps the retro HUD
by ≈120 px vertically. The original does not solve small-window collisions.

### 1.5 Reference rectangles at two resolutions

| Element | 1920×1080 | 1280×720 |
|---|---|---|
| Retro visual | (1475.0, 742.25)–(1904.0, 1064.0) | (835.0, 382.25)–(1264.0, 704.0) |
| Tire panel visual | (1431.212, 379.414)–(1908.788, 732.25) | (791.212, 19.414)–(1268.788, 372.25) |
| Lap panel visual | (26.0, 22.0)–(287.36, 178.816) | same |
| Minimap visual | (35.18, 190.816)–(278.18, 478.816) | same |
| Lap/map block | (14.0, 10.0)–(299.36, 490.816) | same |
| Driving panel | (762.0, 946.0)–(1158.0, 1062.0) | (442.0, 586.0)–(838.0, 702.0) |

---

## 2. Element 1 — Tachometer and speedometer

Owning node `RetroHud` (`RetroHudDisplay`, `Control`). All coordinates are design px; multiply by
0.2962701105 and add `(W−445, H−337.75)` for screen px at 16:9.

### 2.1 Canvas and background

- Design canvas 1448×1086, uniform fit:
  `design_scale = min(size.x/1448, size.y/1086)`, `origin = (size − design·design_scale)/2`.
  With the shipped 390×292.5 control the factor is 0.2693364641 and origin ≈ (0, 0).
- Background: `draw_rect(Rect2(0, 0, size), #06152566)` — RGB (6, 21, 37), **alpha 0.4**, no
  corner radius, no border, no gradient. The world shows through at 60 %.
- Draw order: background → RPM scale → shift lights → gear/speed → numeric RPM → temperatures →
  fuel bar → fuel summary.

### 2.2 Tachometer arc

| Property | Value |
|---|---|
| Centre | (650, 760) design = (1667.6, 967.4) screen @1080p |
| Radius | 470 design = 139.25 px |
| Stroke width | 52 design = 15.41 px (band spans radius 444–496 design) |
| Start angle | 180° (screen left / 9 o'clock) |
| End angle | 328° (up-right / ≈1 o'clock) |
| Sweep | 148°, clockwise on screen (angles increase through 270° = up) |
| Arc tessellation | 128 points per band, antialiased |
| Needle | **none** — the arc is the indicator; there is no needle, hub or pivot |

The arc is **continuous**; there are no per-segment tick marks or gaps.

Bands, drawn in this order:

1. Inactive full band from `rpm_min` (0) to `urgent` (17 800), colour `inactive_color` `#183149`.
2. Active band from 0 to `clamp(rpm, 0, urgent)`, colour `alert_color` `#f0182d` when
   `rpm ≥ urgent`, else `dial_color` `#f4f5f6`. The band starts at 0, so it is a filled progress
   arc, not a floating segment.
3. Redline band from `urgent` (17 800) to `rpm_max` (19 000), colour `redline_color` `#f0182d` when
   `rpm ≥ urgent`, else `inactive_redline_color` `#67202b`. Its angular width is
   `(19000−17800)/19000 × 148° ≈ 9.35°`.

`revolutions_per_minute_redline` (18 000) is **not** drawn; it is only a clamp/scale reference. The
visible red band starts at `urgent_shift_revolutions_per_minute` (17 800).

### 2.3 Scale labels

- `revolutions_per_minute_segment_count = 20` → 19 intervals across `[min, max]`.
- For each segment `i` in `0..19`: `ratio = i/19`, `angle = lerp(180°, 328°, ratio)`,
  `rpm = lerp(min, max, ratio)`, `label = round(rpm/1000)`.
- A label is drawn only when `label` is even (`label % 2 == 0`). With the default 0–19 000 range
  that yields exactly 0, 2, 4, …, 18 (10 labels).
- Label centre = `(650,760) + (cos θ, sin θ) × 540`; the label is centred on that point (both axes).
- Font: D-DIN Condensed **Bold**, size 88 design (≈26.07 px), colour `dial_color` `#f4f5f6`.
- With a car whose `max_rpm ≠ 19 000` the labels can change (see §2.6).

### 2.4 Shift lights

Two circles at the upper-left of the tachometer, drawn under the gear/speed text:

| Light | Centre | Outer ring | Inner disc | Turns on |
|---|---|---|---|---|
| 1 (white) | (130, 130), r 57 / 48 | `inactive_color` `#183149` | `dial_color` `#f4f5f6` | `rpm ≥ ideal_shift` (17 400) |
| 2 (red) | (285, 130), r 57 / 48 | `inactive_color` `#183149` | `alert_color` `#f0182d` | `rpm ≥ urgent_shift` (17 800) |

At/above 17 800 both lights are lit. Lights are not drawn at all below their thresholds (no dark
ring is drawn until the threshold is crossed — the ring and disc appear together).

### 2.5 Gear, speed and numeric RPM

| Item | Centre (design) | Font | Size | Colour |
|---|---|---|---|---|
| Gear label | (617, 540) | D-DIN **Expanded Bold** | 400 | `alert_color` if `rpm ≥ urgent`, else `digital_color` |
| Speed digits | (895, 594) | D-DIN **Bold** | 160 | `digital_color` `#f4f5f6` |
| `KM/H` | (895, 694) | D-DIN Condensed Bold | 52 | `dial_color` |
| RPM digits | (1240, 575) | D-DIN Condensed Bold | 180 | `alert_color` if `rpm ≥ urgent`, else `digital_color` |
| `RPM` | (1240, 700) | D-DIN Condensed Bold | 52 | `dial_color` |

Text is centred on the given point by measured string width; baseline is
`centre.y + (ascent − descent)/2` (see §11.6).

Formats:

- Gear: `"R"` if `current_gear < 0`, `"N"` if `== 0`, else the decimal integer. No padding.
- Speed: `str(int(round(abs(speed_mps) × 3.6)))` — integer km/h, no decimals, no leading zeros, no
  unit in the digits; unit is the separate `KM/H` label. Values can exceed 3 digits.
- RPM: `str(int(round(max(rpm, 0))))` — integer, no padding, no separators.

### 2.6 Car-specific RPM scaling

`CompetitionHudAdapter._synchronize_vehicle_powertrain_configuration()` reads
`vehicle.physics_config_path`, loads the profile, takes `powertrain.max_rpm`, and calls
`RetroHudDisplay.set_powertrain_configuration()`. `RetroHudConfig.apply_powertrain_configuration()`
then applies, once per profile change (the JSON is reloaded first, so the base values reset):

```
engine_limit   = powertrain.max_rpm
if not finite or engine_limit <= rpm_min: no change
threshold_scale = engine_limit / max(rpm_redline_from_json, 1)   # 18000 in the shipped file
rpm_max   = max(rpm_min + 1, rpm_max   × threshold_scale)
ideal    *= threshold_scale
urgent   *= threshold_scale
rpm_redline = engine_limit
```

The shipped Fuji session (`data/vehicles/f1_2030_v10.tres` → geometric profile) has
`max_rpm = 18000`, so `threshold_scale = 1.0` and the gauge stays 0–19 000 / red band
17 800–19 000. A 1997 Williams (`max_rpm = 17500`) would become max ≈ 18 472, ideal ≈ 16 917,
urgent ≈ 17 306, redline 17 500.

Because labels use `round(lerp(min,max,i/19)/1000)` and are drawn only on even values, the label set
is a function of the scaled range: with the shipped 18 000 and 17 500 rpm profiles it remains the
even thousands, but other `max_rpm` values can skip or repeat labels. The port must reproduce the
formula, not a fixed 0..18 list.

### 2.7 Legacy hidden speedometer

`SpeedGauge` (`ArcadeSpeedGauge`) is present but `visible=false`; it never renders in the shipped
HUD. For completeness: 176×110 control anchored bottom-right at `(W−194, H−126)`; arc centre
`(0.42·w, 0.89·h)`, radius `min(0.40·w, 0.82·h)`, sweep 180°→348°, four coloured segments
(yellow (1.0, 0.82, 0.08), orange (1.0, 0.46, 0.05), red (1.0, 0.14, 0.20), purple
(0.41, 0.07, 0.78)), white 2 px needle at `speed/360`, plus `SpeedValue` (28 px), `Unit` (`KPH`,
10 px), `GearPlate` (34×36, `rgba(0.05, 0.08, 0.37, 0.94)`), `GearValue` (22 px,
`rgba(0.9, 1.0, 0.18, 1.0)`). It is **not** part of the port.

---

## 3. Element 2 — Fuel level and amount

All visible fuel information is inside `RetroHud`. The `EngineTemperaturePanel` fuel/avg/delta rows
are **[hidden]** (see §4.4) and must not be reproduced as a second panel.

### 3.1 Fuel bar (level)

| Property | Value |
|---|---|
| Icon | `fuel_icon.svg`, centred at (328, 783), drawn 68×78 design, tinted `dial_color` |
| Track | `Rect2((383, 751), (690, 66))`, filled `fuel_container_color` `#9aa5ae` |
| Segments | `fuel_segment_count = 12`, gap 7, inner horizontal padding 17 each side |
| Segment width | `(690 − 34 − 7×11)/12 = 48.25` |
| Segment height | `66 − 16 = 50`, centred at y = 784 |
| First segment centre x | `383 + 17 + 48.25/2 = 424.125`; step `48.25 + 7 = 55.25` |
| Inactive colour | `fuel_inactive_segment_color` `#4a555e` |
| Active colour | `fuel_active_segment_color` `#f2f3f1` normally; `alert_color` `#f0182d` when the fraction ≤ `fuel_alert_fraction` (0.05) |

Representation: a horizontal **segmented bar**, no needle, no numeric percent. It shows *level*
only. Quantity is the separate numeric readout below.

Fraction: `fuel_fraction = clamp(remaining_kg / capacity_kg, 0, 1)`; it is 0 when
`capacity_kg ≤ 0` or `remaining_kg < 0`.

Each segment is drawn as the full `fuel_segment.svg` texture in the inactive colour, then, if its
fill > 0, the same texture is drawn again with a **left-cropped source region**
(`source width = texture_width × fill`, destination width `= segment_width × fill`) in the active
colour. `fill = clamp(fraction×12 − i, 0, 1)`, so all segments are full except the last partial
one; fill is left-to-right. The SVG shape (60×48, chamfered top corners) is stretched to
48.25×50 (slight aspect distortion); the active overlay is cropped, not rescaled.

### 3.2 Fuel summary (quantity, average, delta)

Labels use D-DIN Condensed Regular; values use D-DIN Condensed Bold.

| Text | Centre (design) | Font size | Colour |
|---|---|---|---|
| `FUEL` | (290, 887) | 52 | `dial_color` |
| fuel value | (292, 960) | 64 | `dial_color`, or `alert_color` if `capacity > 0` and `remaining/capacity ≤ 0.05` |
| `KG` | (408, 968) | 52 | `dial_color` |
| `AVG` | (728, 887) | 52 | `dial_color` |
| avg value | (725, 960) | 64 | `digital_color` |
| `KG/LAP` | (866, 968) | 52 | `dial_color` |
| `DELTA` | (1154, 887) | 64 | `dial_color` |
| delta value | (1078, 960) | 108 | `alert_color` if delta < 0, else `digital_color` |
| `LAPS` | (1305, 968) | 52 | `dial_color` |

Vertical dividers: `divider.svg` drawn as 4×123 at (509, 879) and (943, 879), colour `#ffffff`.
There is no horizontal divider in the fuel row.

Formats and missing values:

| Slot | Present | Missing |
|---|---|---|
| Fuel quantity | `"%.1f" % remaining_kg` + separate `KG` | `"--.-"` at (291, 963), `inactive_color` `#183149` |
| Average | `"%.2f" % average_consumption_kg_per_lap` + `KG/LAP` | `"--.--"` at (724, 963), `inactive_color` |
| Delta | `"%.2f"` for ≥ 0, `"-%.2f" % abs(delta)` for < 0 + `LAPS` | `"--.--"` at (1078, 963), `inactive_color` |

Notes:

- Units are uppercase `KG`, `KG/LAP`, `LAPS` in the retro HUD (the hidden panel uses lowercase
  `kg`, `kg/l`, `laps`).
- The delta has **no plus sign** for positive values; negative values get a leading `-`.
- The delta is red for **any** negative value (the hidden panel uses −0.25 / −1.0 lap thresholds
  instead).
- `remaining_kg`/`capacity_kg` come from `vehicle.get_fuel_state_snapshot()`. `average` and
  `delta` come from `LapTimingController` (`has_consumption_average`,
  `average_consumption_kg_per_lap`, `has_laps_delta`, `laps_delta`), not from the fuel snapshot.
- The average/delta are **not** computed by the HUD; the lap controller computes them from fuel
  samples and the reference lap time.

---

## 4. Element 3 — Engine water/oil temperature

### 4.1 Visible representation (`RetroHud`, right side of the tachometer)

| Text | Centre (design) | Font | Size | Colour |
|---|---|---|---|---|
| `OIL TEMP` | (1257, 123) | D-DIN Condensed Regular | 52 | `dial_color` |
| oil value | (1268, 203) | D-DIN Condensed Regular | 94 | temperature colour (below) |
| `°C` | (1370, 204) | D-DIN Condensed Regular | 52 | `dial_color` |
| horizontal divider | start (1164, 264), length 224 | `divider.svg`, 4 px thick, rotated 90° | — | `#ffffff` |
| `WATER TEMP` | (1257, 304) | D-DIN Condensed Regular | 52 | `dial_color` |
| water value | (1268, 385) | D-DIN Condensed Regular | 94 | temperature colour |
| `°C` | (1370, 386) | D-DIN Condensed Regular | 52 | `dial_color` |

Value format: `str(int(round(temperature_c)))` — integer, no decimals. Unit is a separate `°C`
label. Missing (`temperature < 0`): `"--"` at the value centre, `inactive_color` `#183149`; the
`°C` label remains.

The divider is the 8×128 `divider.svg` texture drawn as a 4×224 rectangle rotated 90° about its
centre `(1276, 264)` (i.e. a 224-long, 4-thick horizontal line from x=1164 to 1388 at y=264).

### 4.2 Temperature colour algorithm (adapter, used for the retro display)

Input: `thermal_state[system]` with `system` ∈ {`"water"`, `"oil"`}, keys `temperature_c`,
`optimal_min_c`, `optimal_max_c`, `derating_c`, `critical_c`.

```
if temperature_c < 0 or system dict is not a Dictionary: cold_color
if not system.has("optimal_min_c"):                     optimal_color
if temperature_c < optimal_min_c:                       cold_color
if temperature_c <= optimal_max_c (default INF):        optimal_color
if temperature_c < derating_c (default INF):            warm_color
if temperature_c < critical_c (default INF):            hot_color
else:                                                   critical_color
```

| State | Colour | Hex |
|---|---|---|
| cold | blue | `#73b8ff` |
| optimal | green | `#a6ff9e` |
| warm | amber | `#ffd16b` |
| hot | orange-red | `#ff6b61` |
| critical | red | `#d91f1f` |

The thresholds are **data-driven** from the snapshot, not hard-coded in the HUD. The shipped
F1 2030 geometric profile declares water 80/105/110/120 °C and oil 95/125/130/145 °C (in
`powertrain.thermal` as `water_optimal_minimum_temperature_celsius`, etc.). The HUD never reads the
profile's thermal block directly; it relies on the snapshot's per-system thresholds.

### 4.3 Missing snapshot

If `get_engine_thermal_state_snapshot()` returns empty/non-dictionary, the adapter falls back to
`get_telemetry_snapshot()["engine_thermal"]`. If that is also empty, both values are `-1` and the
display shows `--` in `inactive_color`.

### 4.4 Hidden `EngineTemperaturePanel` (do not reproduce as a second panel)

`CompetitionHudAdapter._ready()` sets `engine_temperature_panel.visible = false`; the feature
README states this is because its temperatures and fuel figures are part of the retro display. The
panel still exists and is still updated every frame; it is positioned above the tire panel when
visible (`engine_temperatures.gap = 8`). Its content, for reference only:

- PanelContainer, 390×168 [measured size; content minimum height 135], scale 1.188, background
  `#06152566`, margins 16/16/12/12, VBox (default separation 4), title `ENGINE` (18 px, D-DIN
  regular, centred), HBox with `WATER` and `OIL` labels (18 px, expand fill, separation 24), then
  `FUEL`, `AVG`, `DELTA` rows (18 px), all centred.
- Formats: `"WATER  %.1f°"`, `"OIL  %.1f°"`, `"FUEL  %.1f kg"`, `"AVG  %.2f kg/l"`,
  `"DELTA  %+.2f laps"`; missing: `"WATER  ---°"`, `"OIL  ---°"`, `"FUEL  ---"`, `"AVG  --"`,
  `"DELTA  --"`.
- Colours: same five-state palette; fuel ≤ 0.05 critical, ≤ 0.15 low, else green; delta ≤ −1.0
  critical, ≤ −0.25 warning, else green.
- Its thresholds default to `0.0` when the snapshot omits them, which can disagree with the retro
  display's "optimal when `optimal_min_c` is missing" rule. This is one reason not to port the
  hidden panel.

---

## 5. Element 4 — Four wheel temperature sets

Owning node `TireStatusPanel` (`PanelContainer`), runtime-created by `ArcadeRaceHud` and styled by
`CompetitionHudAdapter`.

### 5.1 Panel geometry

| Property | Value |
|---|---|
| Content size | 402×297 [measured] (JSON nominal `size` is 390×196; `PanelContainer` expands to its content minimum) |
| Scale | 1.188 |
| Visual size | 477.576×352.836 |
| Position | centre x = W−250, top = H−700.586 (see §1.4) |
| Background | `StyleBoxFlat`, `bg_color = #06152566`, no border, corner radius 0, no shadow |
| Margins | 16 left/right, 12 top/bottom (unscaled) |
| Title | `TYRES`, D-DIN Condensed Bold 18, centred, white (theme default) |
| Grid | `GridContainer`, 2 columns, h_separation 10, v_separation 6 |
| Cell | VBox, `custom_minimum_size = 180×78`, actual minimum 180×122 [measured] |
| Cell rows | wheel id (21 px), pressure (16), tread (19), carcass (14), brake (14), wear (18), with 4 px VBox separation |
| Row order | FL, FR on row 1; RL, RR on row 2 (config `wheel_order = ["FL","FR","RL","RR"]`, filled row-major) |

Cell content is horizontally centred; the grid is 370 wide inside the 402 panel, so each cell is
exactly 180 wide with a 10 px gap.

### 5.2 Per-wheel content

| Row | Label text | Format | Font | Size | Colour |
|---|---|---|---|---|---|
| wheel id | `FL`/`FR`/`RL`/`RR` | literal | D-DIN Condensed Bold | 20 | theme white |
| pressure | `145 kPa` | `"%.0f kPa"` | D-DIN Condensed Regular | 15 | theme white |
| tread | `92 96 88°` | `"%.0f %.0f %.0f°"` (inner, centre, outer; single spaces; one degree sign at the end) | D-DIN Condensed Bold | 18 | colour of the **hottest** of the three |
| carcass | `CAR 81°` | `"CAR %.0f°"` | D-DIN Condensed Regular | 13 | colour of `carcass_c` |
| brake | `BRK 300°` | `"BRK %.0f°"` | D-DIN Condensed Regular | 13 | colour of `disc_c` (see element 5) |
| wear | `57%` | `"%.0f%%"` of `wear_remaining_fraction × 100` | D-DIN Condensed Bold | 17 | wear colour (below) |

There is **no** gas row and **no** rim row. `gas_c` (tire snapshot) and `rim_c` (brake snapshot)
exist in the physics contract and are written by `telemetry_manager.gd`, but
`tire_status_panel.gd` never reads them. The target requirement adds them; the original has no
layout slot for them.

### 5.3 Temperature thresholds and colours (tread and carcass)

| Condition | State | Colour |
|---|---|---|
| `t < 65` | cold | `#73b8ff` |
| `65 ≤ t ≤ 107` | optimal | `#a6ff9e` |
| `107 < t ≤ 120` | warm | `#ffd16b` |
| `t > 120` | hot | `#ff6b61` |

The comparisons are `t < 65` (cold), `t ≤ 107` (optimal), `t ≤ 120` (warm), else hot; thresholds are
presentation constants from `hud_config.json` (`tires.temperature`), not physics data.

Tread row colour uses `max(inner, centre, outer)`; each zone is **not** coloured individually.
Carcass uses its own value. Pressure is never coloured.

### 5.4 Wear

| Condition | Colour |
|---|---|
| `remaining ≤ 0.30` | `#ff6b61` |
| `0.30 < remaining ≤ 0.60` | `#ffd16b` |
| `remaining > 0.60` | `#a6ff9e` |

Only `wear_remaining_fraction` is read. `wear_inner_fraction`, `wear_center_fraction`,
`wear_outer_fraction` and `wear_grip_scale` are in the contract but unused.

### 5.5 Missing-value behaviour (important trap)

- Before any data: `-- kPa`, `-- -- --°`, `CAR --°`, `BRK --°`, `--%`.
- Missing `disc_c` → `"BRK --°"`, and the colour override is removed (theme white).
- Missing `wear_remaining_fraction` → `"--%"`, coloured **green** (`wear_optimal_color`), i.e. a
  missing value looks healthy.
- Missing `pressure_kpa`, `tread_*_c` or `carcass_c` uses the `0.0` default and renders as `0 kPa`,
  `0 0 0°`, `CAR 0°` (cold blue); these keys have no null guard (only `disc_c` and
  `wear_remaining_fraction` are guarded).
- If a wheel key is absent or the whole snapshot is empty, the cell keeps its previous values
  (no reset), so the panel can show stale data indefinitely.
- The target contract requires explicit `--` for invalid data; do not copy the zero-fill or the
  green missing wear.

### 5.6 Data source

`TireStatusPanel._process()` prefers `vehicle.get_tire_state_snapshot()`; only if that method does
not exist does it use `get_telemetry_snapshot()["tires"]`. (If the method exists but returns `{}`,
there is no fallback.) Brakes likewise: `get_brake_state_snapshot()` preferred, else
`get_telemetry_snapshot()["brakes"]`. The shipped config's wheel keys are `FL`, `FR`, `RL`, `RR`
(the panel iterates `tires.wheel_order`, so a config change also changes the keys it looks up).

---

## 6. Element 5 — Brake disc temperatures

There is no separate brake panel. The four disc temperatures are the `BRK` row of each tire cell:

| Property | Value |
|---|---|
| Text | `"BRK %.0f°"` (integer, degree sign, no `C`) |
| Font | D-DIN Condensed Regular 13 |
| Colour | per-wheel function of `disc_c` and the brake snapshot thresholds |
| Position | fifth row of each cell (below carcass, above wear) |

Colour thresholds (per wheel, taken from the brake snapshot; defaults in parentheses when a key is
absent):

| Condition | Colour |
|---|---|
| `t < optimal_min_c` (400) | `#73b8ff` |
| `optimal_min_c ≤ t ≤ optimal_max_c` (800) | `#a6ff9e` |
| `optimal_max_c < t < fade_start_c` (900) | `#ffd16b` |
| `fade_start_c ≤ t < critical_c` (1100) | `#ff6b61` |
| `t ≥ critical_c` | `#d91f1f` |

`rim_c` is **not** displayed. The brake snapshot also carries `efficiency`,
`natural_cooling_w_k`, `speed_cooling_w_k`, `duct_mass_flow_kg_s`, `duct_drag_n`; none are read by
the HUD. The removed `caliper_c`/`hub_c` states are never read (per the panel's own contract
comment).

The disc temperature is shown only as text, with no bar, gauge, gradient or warning icon. Missing
`disc_c` → `BRK --°` in theme white.

---

## 7. Element 6 — Minimap

Owning node `Minimap` (`TrackMinimapController`, `Control`).

### 7.1 Control and placement

| Property | Value |
|---|---|
| Scene offsets | 54, 312, 162, 440 → size 108×128 |
| Scale | 2.25 (set in `debug_hud.tscn`; test asserts it) |
| Visual size | 243×288 |
| Runtime position | `(35.18, 190.816)` (stage px), i.e. centred under the lap panel |
| `padding_ratio` | 0.10 (export range 0.02–0.24) |
| `background_color` | `Color.TRANSPARENT` (adapter sets it; default is also transparent) |
| `background_margin` | 10 (unused while alpha = 0) |
| Border | none |
| Background | the shared `LapAndMapBackground` ColorRect `#06152566` behind it |

### 7.2 Track data and projection

Source: `session_config.selected_track.map_data` (`TrackMapData`), assigned by
`WorldHudCompositor._on_composition_ready()`; for the shipped session it is
`res://scenes/ui/fuji76_77_hud_map.tres`. Fields: `centerline` (`PackedVector2Array` of world
`(x, z)`), `start_position` (`Vector2`), `start_forward` (`Vector2`), plus `get_bounds()` and
`is_valid_map()` (needs ≥ 3 points).

Projection (`_world_to_map`), all in the control's local coordinates:

```
inset      = min(size.x, size.y) × 0.10                    # 10.8 local px
available  = Rect2(inset, inset, size.x − 2·inset, size.y − 2·inset)   # 86.4 × 106.4
bounds     = min/max of centerline (x,z)
source     = max(bounds.size, 0.001)
scale      = min(available.w / source.x, available.h / source.y)
rendered   = source × scale
origin     = available.centre − rendered/2
map(p)     = origin + (p − bounds.position) × scale
```

- No rotation, no y-flip: world **x → screen x**, world **z → screen y** (z increases downward).
  A car facing −Z has heading (0, −1) and points up.
- Uniform scale (aspect-fit), centred; the track is not stretched.
- The control's 2.25 scale multiplies the whole drawing, including line widths.

Fuji 1976-77 shipped numbers: 69 centerline points; bounds x ∈ [−295.704, 221.901]
(width 517.605 m), z ∈ [−761.346, 922.221] (height 1683.567 m); local scale
0.0631991 px/m; rendered 32.712×106.4 local px (73.60×239.40 screen px), local origin
(37.6439, 10.8); start position projects to local (37.521, 43.480) ≈ screen (119.6, 288.6).
The track is therefore a narrow vertical shape centred in the 243×288 map control.

### 7.3 Track outline

- Build a `PackedVector2Array` from every `centerline` point, then append the first point to close
  the loop. For Fuji the closing chord is 13.08 m (≈0.83 local px), essentially invisible.
- Draw a **shadow** polyline: width 4.0 local px (9.0 screen), colour `(0.01, 0.02, 0.03, 0.92)`
  ≈ `#030508eb`.
- Draw the **outline** polyline on top: width 1.8 local px (4.05 screen), colour
  `(0.97, 0.98, 0.94, 0.98)` ≈ `#f7faf0fa`.
- Both antialiased; no fill, no road-width band, no kerbs, no sector colours.

### 7.4 Start/finish marker

- Position `map_data.start_position`; heading `map_data.start_forward.normalized()` (fallback
  `Vector2.UP` if near zero); side = `(−heading.y, heading.x)`.
- A perpendicular tick from `start − side×6` to `start + side×6` (12 local px = 27 screen px),
  first drawn with a 5.0 local px shadow `#030508eb`, then with a 2.6 local px green
  `(0.28, 1.0, 0.34, 1.0)` ≈ `#47ff57`.
- A filled green circle of radius 3.7 local px (8.33 screen) at the start point.
- The marker is **not** clamped to the map rect (the car is). For Fuji the start point is ~1.95 m
  outside the centerline x-bounds, which is visually negligible.
- The marker comes from the map resource, **not** from the lap-timing authority. If the two
  disagree, the map shows the resource's line.

### 7.5 Car marker

- `center = _world_to_map(Vector2(target.global_position.x, target.global_position.z))`, then
  clamped to the map rect (inset included), so an off-track car pins at the edge.
- Heading: `world_forward = -target.global_basis.z`; `heading = normalize(Vector2(forward.x,
  forward.z))`; fallback `Vector2.UP`.
- Polygon (local px, `f` = heading, `s` = side `(−f.y, f.x)`):
  tip `c + f×6`, rear-left `c − f×4 + s×3.5`, tail notch `c − f×2`, rear-right `c − f×4 − s×3.5`.
  Overall ≈ 10 local px long × 7 wide (22.5 × 15.75 screen).
- Fill `(0.20, 0.50, 1.0, 1.0)` ≈ `#3380ff`; outline polyline width 1.8 local px (4.05 screen),
  colour `(0.01, 0.02, 0.08, 0.98)` ≈ `#030514fa`.
- The marker is an arrow/chevron, not a dot or a car sprite.

### 7.6 Update behaviour

`_process()` re-resolves `target_path` if the target is invalid and calls `queue_redraw()` every
frame. `NOTIFICATION_RESIZED` also redraws. The projection is recomputed from the current control
size on every draw, so resizing the control rescales the map; the shipped control size is fixed.

---

## 8. Element 7 — Lap timing

Owning node `LapTimingPanel` (`PanelContainer`), runtime-created; bound to `LapTimingController`.

### 8.1 Panel geometry

| Property | Value |
|---|---|
| Content size | 220×132 [measured] (JSON nominal `size` 220×118; content expands the height) |
| Scale | 1.188 |
| Visual size | 261.36×156.816 |
| Position | (26, 22) stage px, inside the block at (14, 10) with 12 px padding |
| Background | `StyleBoxEmpty` (transparent); the block ColorRect `#06152566` provides the background |
| Border / radius | none |
| Margins | 14 left/right, 10 top/bottom |
| VBox separation | 6 |
| Title | `LAP TIMING`, D-DIN Condensed Bold 18, centred, white |
| Rows | `LAP`, `LAST`, `BEST`; HBox; caption left, value right |

Row layout (unscaled panel px): title y 10–29; row 1 y 35–60; row 2 y 66–91; row 3 y 97–122. Caption
x 14–72 (`caption_width = 58`), value right-aligned to x 206. Row height is 25 (driven by the 24 px
value font). Caption font D-DIN Condensed Regular 15; value font D-DIN Condensed Bold 24, right
aligned, expand fill.

### 8.2 Rows, formats, colours

| Row | Caption | Value expression | Present colour | Missing text | Missing colour |
|---|---|---|---|---|---|
| LAP | `LAP` | `str(current_lap_number)` | `best_lap_color` `#a6ff9e` if a best lap exists, else `pending_color` `#cfd6e4` | `--` when the controller is null/not configured | `pending_color` |
| LAST | `LAST` | `format_lap_time(last_lap_time_seconds)` | `last_lap_color` `#ffffff` | `--:--.---` | `pending_color` |
| BEST | `BEST` | `format_lap_time(best_lap_time_seconds)` | `best_lap_color` `#a6ff9e` | `--:--.---` | `pending_color` |

Quirks:

- The **LAP number is coloured by best-lap availability**, not by lap validity. Before the first
  completed lap it is grey even though the number is valid.
- There is **no current lap time row**. `LapTimingController.current_lap_time_seconds` is maintained
  but never read by the panel. The target requirement asks for "tiempo actual"; it must be added
  from the destination's race authority, not from a new HUD timer.
- There is no position, lap count total, sector, delta-to-best or countdown in this panel.
- `LAP` shows `0` (not `--`) after the controller is configured but before the first start/finish
  crossing, because `current_lap_number` starts at 0. `--` is only used when the controller is
  null or `is_configured == false`.

### 8.3 Time format

`LapTimingController.format_lap_time(seconds)`:

```
if seconds < 0: return "--:--.---"
total_ms = int(round(seconds × 1000))
minutes  = total_ms / 60000        # integer division
seconds  = (total_ms / 1000) % 60
millis   = total_ms % 1000
return "%d:%02d.%03d" % [minutes, seconds, millis]
```

So `m:ss.mmm`, minutes unpadded (can exceed 59), seconds two digits, milliseconds three digits.
Examples: 83.456 → `1:23.456`; 0 → `0:00.000`; 59.9995 → `1:00.000`; negative → `--:--.---`.

### 8.4 Source

`LapTimingController` properties read by the panel: `is_configured`, `current_lap_number`,
`last_lap_time_seconds`, `best_lap_time_seconds`, `format_lap_time()`. `last_lap_time_seconds` and
`best_lap_time_seconds` are `-1.0` until set. The controller's `current_lap_time_seconds`,
`total_time_seconds`, `lap_count` and `planned_laps` are not read by this panel.

---

## 9. Element 8 — Steering indicator

Owning node `DrivingControlsPanel` (`Control`, runtime-created). Local coordinate space is
396×116; the whole control is scaled and positioned as in §1.4.

### 9.1 Panel

| Property | Value |
|---|---|
| Size | 396×116 |
| Scale | `clamp(stage_w/1280, 0.65, 1.0)` |
| Position | `((W − 396·s)/2, H − 116·s − 18)` |
| Background | `StyleBoxFlat` `rgba(0.025, 0.035, 0.05, 0.90)` |
| Border | 1 px `rgba(0.45, 0.52, 0.62, 0.45)` |
| Corner radius | 12 (all corners) |
| Shadow / gradient | none |

### 9.2 Steering wheel

| Item | Value |
|---|---|
| Centre | (62, 64) local |
| Rim | circle radius 27, stroke 3, colour `(0.92, 0.95, 1.0)` |
| Spokes | three lines at base angles −90°, 30°, 150°, each rotated by `wheel_rotation`, from radius 6 to 25, stroke 3 |
| Spoke colours | the −90° spoke `#ffd477`; the other two `(0.70, 0.76, 0.85)` |
| Hub | filled circle radius 6, colour `(0.45, 0.51, 0.61)` |
| Rotation | `−steering × 0.75π` = ±135° at full lock (positive steering → counter-clockwise) |
| `VOLANTE` label | baseline (26, 21), 13 px, colour `(0.78, 0.83, 0.90)`, left-aligned |
| Steering text | baseline (16, 106), box width 92, centred, 13 px, `#ffd477` |

Steering text: `"%d%%" % roundi(abs(steering_fraction) × 100)`, prefixed with `"IZQ "` when
`steering > 0.005`, `"DER "` when `steering < −0.005`, no prefix inside ±0.005. So the original's
representation is **percentage + direction word**, not degrees and not tick marks. There are no
tick marks, no scale, no numeric angle.

Sign convention in the original: **positive = left (`IZQ`)**, negative = right (`DER`); the wheel
rotates counter-clockwise for positive. The destination's steering sign must be mapped explicitly
(the migration notes mention a sign correction in the destination's physics adapter).

The ±135° wheel rotation is a **visual amplification** for the icon; it is not the physical steering
wheel angle and must not be presented as degrees in the port.

### 9.3 Visibility

`DrivingControlsPanel.visible = false` when `_vehicle` is null/invalid; otherwise true. It reads
`get_true_steering_amount()` if the method exists (else 0.0), clamps to [−1, 1], and calls
`set_controls(steering, throttle, brake)` every frame.

---

## 10. Element 9 — Throttle/brake pedal indicators

Same panel as the steering indicator. Two horizontal bars, left-to-right fill.

| Item | Accelerator | Brake |
|---|---|---|
| Label | `ACELERADOR` | `FRENO` |
| Label position | x=126, baseline y=30, 14 px, colour `(0.88, 0.92, 0.98)` | x=126, baseline y=77, same |
| Percent text | `"%d%%" % roundi(fraction×100)`, right-aligned box x=320 width 56, 15 px | same, at baseline 77 |
| Percent colour | `#48df94` | `#ff665c` |
| Bar rect | (126, 39, 250, 15) | (126, 86, 250, 15) |
| Bar background | `(0.16, 0.20, 0.26)` | same |
| Fill | left→right, width `250 × fraction`, colour `#48df94` | `#ff665c` |
| Fraction | `clamp(throttle_input, 0, 1)` | `clamp(brake_input, 0, 1)` |

The bars are horizontal (not vertical), have no border, no gradient, no rounded corners, and no
tick marks. The percent text sits above the right end of each bar, on the same baseline as the pedal
label (x box 320–376, baseline 30 for the accelerator and 77 for the brake).

Inputs are the vehicle's `throttle_input` / `brake_input` properties (clamped 0–1). The original
does not distinguish applied input from driver input or aid intervention; the destination must
document which stage it reads.

---

## 11. Shared visual language

### 11.1 Fonts

The original uses the D-DIN family from `res://fonts/` (TTF, no synthetic weights). The feature
README's "Barlow Condensed Medium" claim is **stale**; the code uses D-DIN.

| File | Bytes | Used by | Sizes (px) |
|---|---|---|---|
| `D-DIN.ttf` | 49 276 | `CompetitionHudAdapter` theme default; hidden engine panel | 18 (default) |
| `D-DIN-Bold.ttf` | 52 204 | retro speed digits | 160 |
| `D-DINCondensed.ttf` | 50 120 | retro labels/values; tire panel; lap panel | 13, 15, 52, 64, 94 |
| `D-DINCondensed-Bold.ttf` | 50 116 | retro scale labels/units/values; tire headers/zones/wear; lap title/values | 17, 18, 20, 24, 52, 64, 88, 108, 180 |
| `D-DINExp-Bold.ttf` | 42 600 | retro gear | 400 |
| `D-DINExp.ttf` | 43 712 | unused | — |

Metrics (Godot 4.7.1, px) for reference: D-DIN/D-DIN Condensed height = 1.01 × size
(18 → 19, 20 → 21, 24 → 25, 52 → 53, 64 → 65, 88 → 89, 94 → 95, 108 → 109, 160 → 161,
180 → 181, 400 → 401); D-DIN Expanded Bold height = 1.10 × size (400 → 435). Ascents/descents are
in the same ratio (e.g. 400: ascent 333, descent 68).

The destination requirement is **Libron Regular/Bold for all migrated HUD text**, so these sizes are
reference values only: keep the anchor points and re-measure widths/cap heights with Libron. The
retro HUD centres every string on a point, so a font-width change moves both edges symmetrically.

### 11.2 Colours (exact)

| Name | Hex | RGBA (0–1) | Used for |
|---|---|---|---|
| background | `#06152566` | 0.023529, 0.082353, 0.145098, 0.400 | retro panel, tire panel, lap/map block |
| dial | `#f4f5f6` | 0.956863, 0.960784, 0.964706, 1 | tach labels, units, fuel labels, temp labels |
| digital | `#f4f5f6` | same | speed/RPM/gear normal, avg value |
| inactive | `#183149` | 0.094118, 0.192157, 0.286275, 1 | tach inactive band, shift-light ring, missing text |
| redline | `#f0182d` | 0.941176, 0.094118, 0.176471, 1 | redline band when lit, alert text |
| inactive redline | `#67202b` | 0.403922, 0.125490, 0.168627, 1 | redline band when not lit |
| alert | `#f0182d` | same as redline | fuel alert, redline, red shift light, negative delta |
| divider | `#ffffff` | 1, 1, 1, 1 | dividers |
| fuel container | `#9aa5ae` | 0.603922, 0.647059, 0.682353, 1 | fuel track |
| fuel inactive | `#4a555e` | 0.290196, 0.333333, 0.368627, 1 | empty fuel segments |
| fuel active | `#f2f3f1` | 0.949020, 0.952941, 0.945098, 1 | filled fuel segments |
| cold | `#73b8ff` | 0.450980, 0.721569, 1.0, 1 | tire/brake/engine cold |
| optimal | `#a6ff9e` | 0.650980, 1.0, 0.619608, 1 | optimal, best lap, wear ok, fuel ok |
| warm | `#ffd16b` | 1.0, 0.819608, 0.419608, 1 | warm, wear warn, fuel low |
| hot | `#ff6b61` | 1.0, 0.419608, 0.380392, 1 | hot, wear critical, fuel critical |
| critical | `#d91f1f` | 0.850980, 0.121569, 0.121569, 1 | brake critical, engine critical |
| pending | `#cfd6e4` | 0.811765, 0.839216, 0.894118, 1 | lap pending |
| last lap | `#ffffff` | 1, 1, 1, 1 | last lap value |
| track outline | (0.97, 0.98, 0.94, 0.98) | — | minimap track |
| track shadow | (0.01, 0.02, 0.03, 0.92) | — | minimap shadow |
| start | (0.28, 1.0, 0.34, 1.0) | — | start tick + dot |
| player | (0.20, 0.50, 1.0, 1.0) | — | car marker fill |
| player outline | (0.01, 0.02, 0.08, 0.98) | — | car marker outline |
| accel | `#48df94` | 0.282353, 0.874510, 0.580392, 1 | accelerator bar/text |
| brake | `#ff665c` | 1.0, 0.400000, 0.360784, 1 | brake bar/text |
| steering | `#ffd477` | 1.0, 0.831373, 0.466667, 1 | steering text and top spoke |
| drive panel bg | (0.025, 0.035, 0.05, 0.90) | — | driving panel |
| drive panel border | (0.45, 0.52, 0.62, 0.45) | — | driving panel |
| drive text | (0.88, 0.92, 0.98) | — | pedal labels |
| drive label | (0.78, 0.83, 0.90) | — | `VOLANTE` |
| bar background | (0.16, 0.20, 0.26) | — | pedal bar track |
| wheel rim | (0.92, 0.95, 1.0) | — | steering wheel rim |
| wheel gray spoke | (0.70, 0.76, 0.85) | — | two spokes |
| wheel hub | (0.45, 0.51, 0.61) | — | hub |

### 11.3 Panels, opacity, radii, borders, gradients

- Retro HUD: full-rect fill `#06152566` (40 % opacity), **no** radius/border/gradient.
- Tire panel: `StyleBoxFlat` `#06152566`, no radius/border.
- Lap panel: transparent (`StyleBoxEmpty`); the block behind it is `#06152566`.
- Minimap: transparent; no border.
- Driving panel: 90 % opacity, 1 px border, 12 px radius.
- No gradients anywhere in these nine elements. No drop shadows; the minimap "shadow" is a thicker
  dark polyline, not a blurred shadow.

### 11.4 Missing-value conventions (all different in the original)

| Element | Placeholder | Colour |
|---|---|---|
| Oil/water (retro) | `--` + `°C` | inactive `#183149` |
| Fuel quantity | `--.-` + `KG` | inactive |
| Average | `--.--` + `KG/LAP` | inactive |
| Delta | `--.--` + `LAPS` | inactive |
| Tire pressure | `-- kPa` (only before data) | theme white |
| Tread | `-- -- --°` (only before data) | theme white |
| Carcass | `CAR --°` (only before data) | theme white |
| Brake | `BRK --°` | theme white |
| Wear | `--%` | **green** `#a6ff9e` |
| Lap number | `--` | pending |
| Lap times | `--:--.---` | pending |
| Hidden engine panel | `WATER  ---°`, `OIL  ---°`, `FUEL  ---`, `AVG  --`, `DELTA  --` | white |

The target contract requires `--` for absent/invalid data with explicit validity; do not copy the
zero-fill, the green missing wear, or the stale-value behaviour.

### 11.5 Threshold summary

| Quantity | Thresholds | Source |
|---|---|---|
| RPM | min 0, ideal 17 400, urgent 17 800, redline 18 000, max 19 000 (all scaled by `max_rpm/18000` when a car profile is loaded) | `retro_hud.json` + `powertrain.max_rpm` |
| Fuel | alert ≤ 5 % (retro); low ≤ 15 %, critical ≤ 5 % (hidden panel) | `retro_hud.json`, `hud_config.json` |
| Tire temperature | cold < 65, optimal ≤ 107, warm ≤ 120, hot > 120 °C | `hud_config.json` (presentation constants) |
| Tire wear | critical ≤ 30 %, warn ≤ 60 % | `hud_config.json` |
| Brake disc | cold < 400, optimal ≤ 800, warm < 900, hot < 1100, critical ≥ 1100 °C | brake snapshot, defaults 400/800/900/1100 |
| Engine water/oil | optimal_min/optimal_max/derating/critical from the snapshot (F1 2030: water 80/105/110/120, oil 95/125/130/145) | thermal snapshot |
| Lap delta (hidden panel only) | warning ≤ −0.25, critical ≤ −1.0 laps | `hud_config.json` |
| Speed (hidden gauge only) | 360 km/h full scale | `arcade_speed_gauge.gd` |

### 11.6 Text rendering rule

`_draw_competition_text(text, centre, size, colour, font)`:

```
width    = font.get_string_size(text, LEFT, -1, size).x
baseline = (centre.x − width/2, centre.y + (font.get_ascent(size) − font.get_descent(size))/2)
draw_string(font, baseline, text, LEFT, -1, size, colour)
```

So the given coordinate is the **centre of the string's em box**, not a top-left or baseline
anchor. Every table above gives those centres. The port must reproduce this anchoring (or convert
to its own baseline convention) to keep the layout.

---

## 12. Duplicated or synthetic data (do not reproduce)

1. **Speed/RPM stored twice.** `RetroHudState` has `speed_kph` and `speed_kilometers_per_hour`,
   `rpm` and `engine_revolutions_per_minute`; both pairs are written with the same values in both
   `set_readout()` and `set_competition_readout()`. The display reads only the long names.
2. **Two update paths per frame.** `CompetitionHudAdapter._update_speed_gauge()` calls
   `super._update_speed_gauge()` (which calls `retro_hud.set_readout(speed, rpm, gear, throttle,
   brake)`) and then calls `retro_hud.set_competition_readout(...)` with the same speed/rpm/gear.
   This can emit `changed` twice per frame. One update path is enough in the port.
3. **Fuel/avg/delta and water/oil are duplicated in the hidden `EngineTemperaturePanel`**, with
   different strings, units and thresholds. The retro HUD is the visible source of truth.
4. **Speed/gear are duplicated in the hidden `SpeedGauge`** (`SpeedValue`, `GearPlate/GearValue`).
5. **Throttle/brake are pushed into `RetroHudState` but never drawn there**; only
   `DrivingControlsPanel` draws them.
6. **`_peak_rpm` is tracked but never rendered**; `speed_segments` and `peak_activation_rpm` are
   config fields never used; `REVOLUTIONS_PER_MINUTE_SEGMENT_TEXTURE` (`rpm_segment.svg`) is
   preloaded but never drawn.
7. **`current_lap_time_seconds` is maintained but never displayed** (the target must add it).
8. **`LAP` colour is keyed to best-lap availability**, not to lap-number validity.
9. **The tire panel's missing numeric keys become 0**, and missing wear is green; these are
   synthetic/invalid states, not data.
10. **The minimap start marker comes from the map resource**, not from the lap-timing start/finish
    authority; do not treat it as verified timing data.
11. **The README is stale** (Barlow Condensed, "segmented" scale). The code is the truth.

---

## 13. Asset inventory (textures/atlases used by these panels)

No atlases. Every graphic is an independent SVG imported as a `CompressedTexture2D`
(lossless, no mipmaps, `svg/scale=1.0`).

| Path | Source size (px) | Bytes | Used by | Purpose |
|---|---|---|---|---|
| `res://features/retro_hud/assets/fuel_icon.svg` | 96×112 | 419 | `RetroHudDisplay._draw_fuel_bar()` | Fuel pump icon at (328, 783), drawn 68×78, tinted `dial_color`; the SVG has a dark inner window `#061525` |
| `res://features/retro_hud/assets/fuel_segment.svg` | 60×48 | 140 | `RetroHudDisplay._draw_fuel_bar()` / `_draw_fuel_segment()` | One fuel bar segment (chamfered top corners); drawn 12× at 48.25×50, inactive full + active left-cropped |
| `res://features/retro_hud/assets/divider.svg` | 8×128 | 134 | `RetroHudDisplay._draw_vertical_divider()` / `_draw_horizontal_divider()` | 4×123 vertical dividers at x=509 and x=943; 4×224 horizontal divider at (1164, 264), rotated 90° |
| `res://features/retro_hud/assets/rpm_segment.svg` | 100×48 | 148 | **nothing** | Preloaded as `REVOLUTIONS_PER_MINUTE_SEGMENT_TEXTURE` but never drawn (the tachometer is procedural arcs) |
| `res://features/retro_hud/assets/digital_segment.svg.import` | — | 1 072 | **nothing** | Orphan import: `source_file="res://features/retro_hud/assets/digital_segment.svg"` is missing; the `.ctex` path is `.godot/imported/digital_segment.svg-22445be792ac834ec06ae5cf49309647.ctex` |
| `res://fonts/D-DIN.ttf` | — | 49 276 | adapter theme, hidden engine panel | UI regular |
| `res://fonts/D-DIN-Bold.ttf` | — | 52 204 | retro speed | Speed digits |
| `res://fonts/D-DINCondensed.ttf` | — | 50 120 | retro/tire/lap | Regular labels and values |
| `res://fonts/D-DINCondensed-Bold.ttf` | — | 50 116 | retro/tire/lap | Bold labels and values |
| `res://fonts/D-DINExp-Bold.ttf` | — | 42 600 | retro gear | Gear glyph |
| `res://scenes/ui/fuji76_77_hud_map.tres` | — | 1 603 | minimap | Track map **data** (not a texture): 69-point centerline, start position/forward |

The minimap itself draws no texture; it uses `draw_polyline`, `draw_line`, `draw_circle` and
`draw_colored_polygon`. The tire/lap/driving panels draw text and boxes only. The only textures in
the nine elements are the three fuel/divider SVGs above.

---

## 14. Porting notes (differences the destination requirements introduce)

These are not changes to the original; they are the deltas the destination contract already
requires, recorded here so the layout/data mapping can account for them.

1. **Font.** Replace D-DIN with Libron Regular/Bold. Keep the anchor centres and re-measure widths;
   the retro strings are centre-anchored, so widths change symmetrically. The driving panel's
   `ThemeDB.fallback_font` is not Libron; the target must use Libron everywhere.
2. **Steering in degrees.** The original shows `%` + `IZQ`/`DER`; the destination requires wheel
   degrees from the effective steering and the configured range/ratio. The original's ±135° icon
   rotation is a visual amplification and must not be presented as a physical angle.
3. **Current lap time.** The original panel has no current-time row; the destination requires one
   from the race authority. Do not add a second timer.
4. **Gas and rim temperatures.** The original HUD has no slot; the destination requires them when
   the model publishes them. The panel is already at its content-driven 402×297 size, so adding two
   rows will grow it; plan the layout explicitly.
5. **Wheel order.** The original grid is FL, FR / RL, RR. The destination's Rust order is
   FL/FR/RL/RR and Speed Dreams uses FR/FL/RR/RL; centralize the mapping.
6. **Missing data.** Use explicit `--`; do not reproduce zero-filled missing numeric keys, green
   missing wear, or stale values.
7. **RPM limits.** Scale the gauge and thresholds from the active car's `max_rpm`, not from the
   F1 2030 18 000/19 000 reference.
8. **Fuel units.** The original is kg; if the destination converts to litres, use the profile
   density and label the unit explicitly.
9. **Tire/brake thresholds.** Tire thresholds are presentation constants (65/107/120); brake
   thresholds come from the brake snapshot with 400/800/900/1100 defaults. Prefer the active
   profile's physical thresholds where available, without duplicating physics constants.
10. **Minimap.** Do not reuse the Fuji `.tres` coordinates for the destination track; build the
    polyline from the destination's track and validate orientation, start/finish and several car
    positions. The original projection is a plain XZ→XY aspect-fit with z down and no rotation.
11. **Layout.** The right group is fixed-size: retro 429×321.75, tire 477.6×352.8, lap 261.4×156.8.
    At 720p the tire panel top is 19.4 px; below ~697 px stage height it overlaps the retro HUD.
    At 1280 width the driving panel overlaps the retro HUD by 3 px; below that it grows. The
    destination must scale/shift for small viewports, which the original does not.
12. **Aspect policy.** The original HUD is inside a 16:9 letterboxed stage. If the destination uses
    the full window, recompute the anchors against the real visible area, not the 16:9 stage.
13. **No duplicated widgets.** Do not port the hidden `EngineTemperaturePanel`, the hidden
    `SpeedGauge`, `PitStopPanel`, `AidMessage` or `HandlingTuningPanel`.

---

## 15. Data keys used

This is the complete list of external keys/expressions read by the nine elements. The paragraph
"present in the contract but not read" lists keys that exist in the documented snapshot contract but
are **not** read by the HUD.

### 15.1 Vehicle scalar properties (`Node.get`)

| Key | Unit/type | Read by | Used for |
|---|---|---|---|
| `speed` | float, m/s | `ArcadeRaceHud._update_speed_gauge`, `CompetitionHudAdapter._update_speed_gauge` | `abs(speed) × 3.6` → km/h |
| `current_gear` | int | same | `R` / `N` / digit |
| `motor_rpm` | float, rpm | same | tachometer arc, numeric RPM, shift lights, gear colour |
| `throttle_input` | float 0–1 | `ArcadeRaceHud._update_speed_gauge`, `_update_driving_controls` | accelerator bar; also stored in `RetroHudState` (unused by the display) |
| `brake_input` | float 0–1 | same | brake bar; same state note |
| `physics_config_path` | String | `CompetitionHudAdapter._synchronize_vehicle_powertrain_configuration` | loads the profile for `powertrain.max_rpm` |

### 15.2 Vehicle methods and nested snapshot keys

| Method | Nested keys read | Consumer |
|---|---|---|
| `get_true_steering_amount()` | return float [−1, 1] | `DrivingControlsPanel` |
| `get_engine_thermal_state_snapshot()` | `["water"]["temperature_c"]`, `["water"]["optimal_min_c"]`, `["water"]["optimal_max_c"]`, `["water"]["derating_c"]`, `["water"]["critical_c"]`; the same five under `["oil"]` | `CompetitionHudAdapter`, `EngineTemperaturePanel` |
| `get_telemetry_snapshot()` | `["engine_thermal"]` (fallback thermal); `["tires"]` (fallback tire); `["brakes"]` (fallback brake) | adapter, tire panel, engine panel |
| `get_fuel_state_snapshot()` | `["remaining_kg"]`, `["capacity_kg"]` | adapter, engine panel, `LapTimingController._sample_remaining_fuel_kg()` |
| `get_tire_state_snapshot()` | per wheel `FL`/`FR`/`RL`/`RR`: `["pressure_kpa"]`, `["tread_inner_c"]`, `["tread_center_c"]`, `["tread_outer_c"]`, `["carcass_c"]`, `["wear_remaining_fraction"]` | `TireStatusPanel` |
| `get_brake_state_snapshot()` | per wheel: `["disc_c"]`, `["optimal_min_c"]`, `["optimal_max_c"]`, `["fade_start_c"]`, `["critical_c"]` | `TireStatusPanel` |

**Tire snapshot keys present in the contract but not read:** `gas_c`, `wear_inner_fraction`,
`wear_center_fraction`, `wear_outer_fraction`, `wear_grip_scale`, `schema_version`.
**Brake snapshot keys present but not read:** `rim_c`, `efficiency`, `natural_cooling_w_k`,
`speed_cooling_w_k`, `duct_mass_flow_kg_s`, `duct_drag_n`, `schema_version`.
**Fuel snapshot:** `schema_version` present, not read.

### 15.3 Lap timing authority (`LapTimingController`)

| Property/method | Type | Read by | Used for |
|---|---|---|---|
| `is_configured` | bool | `LapTimingPanel._refresh` | `--` state |
| `current_lap_number` | int | `LapTimingPanel` | LAP row |
| `last_lap_time_seconds` | float (s, −1 = none) | `LapTimingPanel` | LAST row |
| `best_lap_time_seconds` | float (s, −1 = none) | `LapTimingPanel` | BEST row and LAP colour |
| `format_lap_time(seconds)` | String | `LapTimingPanel` | `m:ss.mmm` / `--:--.---` |
| `has_consumption_average` | bool | adapter, engine panel | AVG availability |
| `average_consumption_kg_per_lap` | float kg/lap | adapter, engine panel | AVG value |
| `has_laps_delta` | bool | adapter, engine panel | DELTA availability |
| `laps_delta` | float laps | adapter, engine panel | DELTA value |
| `current_lap_time_seconds` | float | **not read by the HUD** | target must add current time |
| `total_time_seconds`, `lap_count`, `planned_laps`, `instant_consumption_kg_per_lap`, `has_instant_consumption` | — | **not read by the HUD** | — |

### 15.4 Map data (`TrackMapData`, `res://scenes/ui/fuji76_77_hud_map.tres`)

| Key | Type | Consumer | Used for |
|---|---|---|---|
| `centerline` | `PackedVector2Array` of world `(x,z)` | `TrackMinimapController._draw` | closed track polyline; `get_bounds()` |
| `start_position` | `Vector2` (x,z) | `_draw_start_marker` | start/finish marker |
| `start_forward` | `Vector2` (x,z) | `_draw_start_marker` | tick orientation |
| `get_bounds()` | `Rect2` | `_world_to_map` | projection bounds |
| `is_valid_map()` | bool (`centerline.size() >= 3`) | `_draw` | draw guard |

The map resource is reached through `RaceSessionConfig.selected_track.map_data`
(`WorldHudCompositor._on_composition_ready`).

### 15.5 Vehicle transform (minimap)

| Key | Type | Consumer |
|---|---|---|
| `global_position` (x, z) | Vector3 | `TrackMinimapController.get_player_map_position` |
| `global_basis.z` | Vector3 | `_draw_player_marker` heading (`−basis.z`) |

### 15.6 Configuration keys

`res://features/retro_hud/config/retro_hud.json` (read by `RetroHudConfig`):

`revolutions_per_minute_minimum`, `revolutions_per_minute_maximum`,
`revolutions_per_minute_redline`, `ideal_shift_revolutions_per_minute`,
`urgent_shift_revolutions_per_minute`, `revolutions_per_minute_segment_count`,
`fuel_segment_count`, `fuel_alert_fraction`, `fuel_container_color`,
`fuel_inactive_segment_color`, `fuel_active_segment_color`, `background_color`, `dial_color`,
`digital_color`, `inactive_color`, `redline_color`, `inactive_redline_color`, `alert_color`,
`divider_color`, `scale`, `visible`. (`version` and `style` are present but ignored.)

`res://features/hud/config/hud_config.json` (read by `HudConfig`):

| Section | Keys read |
|---|---|
| `retro_hud` | `visible`, `scale`; `config_path` is present but unused |
| `tires` | `visible`, `scale`, `size`, `gap`, `title`, `title_font_size`, `wheel_font_size`, `tread_font_size`, `wear_font_size`, `pressure_font_size`, `detail_font_size`, `box_size`, `margin_left`, `margin_right`, `margin_top`, `margin_bottom`, `h_separation`, `v_separation`, `wheel_order`, `temperature.cold_max_c`, `temperature.optimal_max_c`, `temperature.warm_max_c`, `temperature.cold_color`, `temperature.optimal_color`, `temperature.warm_color`, `temperature.hot_color`, `brake.cold_color`, `brake.optimal_color`, `brake.warm_color`, `brake.hot_color`, `brake.critical_color`, `wear.warn_remaining_fraction`, `wear.critical_remaining_fraction`, `wear.optimal_color`, `wear.warn_color`, `wear.critical_color` |
| `engine_temperatures` | `visible`, `scale`, `size`, `gap`, `title`, `margin_left`, `margin_right`, `margin_top`, `margin_bottom`, `column_separation`, `cold_color`, `optimal_color`, `warm_color`, `hot_color`, `critical_color`, `fuel_color`, `fuel_low_color`, `fuel_critical_color`, `fuel_low_fraction`, `fuel_critical_fraction`, `delta_neutral_color`, `delta_warning_color`, `delta_critical_color`, `delta_warning_laps`, `delta_critical_laps` |
| `lap_timing` | `visible`, `scale`, `size`, `title`, `title_font_size`, `caption_font_size`, `value_font_size`, `margin_left`, `margin_right`, `margin_top`, `margin_bottom`, `row_separation`, `caption_width`, `map_gap`, `block_padding`, `pending_color`, `last_lap_color`, `best_lap_color` |
| `pit_stop` | present; out of scope |

`retro_hud` section keys read by the adapter for layout: `display_scale` (= `scale`), `visible`;
`lap_timing.margin_left`, `lap_timing.margin_top`, `lap_timing.block_padding`, `lap_timing.map_gap`;
`tires.gap`; `engine_temperatures.gap`.

### 15.7 Powertrain

| Source | Key | Consumer |
|---|---|---|
| vehicle profile (`physics_config_path` → `load_configuration`) | `powertrain.max_rpm` | `RetroHudConfig.apply_powertrain_configuration` → gauge max/ideal/urgent/redline |

The profile's `powertrain.thermal` block (water/oil thresholds) is **not** read by the HUD; the
thresholds arrive through the thermal snapshot.

### 15.8 Session configuration

| Key | Consumer |
|---|---|
| `RaceSessionConfig.selected_track.map_data` | minimap map resource |
| `RaceSessionConfig.selected_vehicle`, `selected_track` | session setup (not read by the HUD) |

---

*End of specification.*
