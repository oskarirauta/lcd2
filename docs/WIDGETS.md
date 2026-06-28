# Widget Reference

Widgets are defined in the config file as `widget:name { }` blocks. Every widget
requires a `type` key selecting one of: `ttf`, `bar`, `linechart`, `curvechart`,
`gauge`, `clock`, `image`.

Defaults, types and ranges below are taken directly from each widget's
constructor (`allowed_keys`) and `render()` in the source. Colors accept hex
`RGB`/`RGBA`; values starting with a digit must be quoted (e.g. `'44cc44'`),
all-letter colors (e.g. `ffffff`) may be unquoted.

> **`center` placement:** a widget with `center 1` re-canvases to the **full
> display width** and centers its content there, so it should be placed at
> **x = 0** in the layout. Placing a centered widget at a non-zero x pushes it
> partly off-screen (you will see `add_pixel: x/y out of bounds` warnings).

---

## TTF — TrueType text

Renders a text string using a TrueType font (libgd `gdImageStringTTF`). The
canvas is auto-sized to the text unless `width`/`height` are given.

```
widget:w_hostname {
    type    ttf
    font    '/usr/share/fonts/MyFont.ttf'
    size    16
    color   ffffff
    text    uname::hostname()
    update  60000
    reload  1
}
```

### Recognized keys

`allowed_keys` (verbatim): `text`, `font`, `size`, `color`, `width`, `height`,
`align`, `offset`, `scale`, `inverted`, `opacity`, `center`, `debugborder`,
`debugbordercolor`, `shadow`, `shadowcolor`, `shadowoffset`, `outline`,
`outlinecolor`, `visible`, `reload`, `interval`, `class`, `type`.

The config key `update` is accepted as an alias for `interval`.

| Key | Type | Default | Description |
|---|---|---|---|
| `text` | string (expression) | — (required) | Text to render. Evaluated as an expression. Widget fails to initialize if missing/empty. |
| `font` | string (path) | — (required) | Path to a `.ttf` font file. Must exist and be readable, or rendering is skipped. |
| `size` | number | `12.0` | Font size in points. |
| `color` | hex RGB(A) | `ffffff` | Text color. Required and validated; an invalid value at render time falls back to `ffffff`. |
| `width` | int | `0` (auto) | Canvas width. `0` auto-sizes to the text; `>0` fixes width and enables `align`. |
| `height` | int | `0` (auto) | Canvas height. `0` auto-sizes to the text height. |
| `align` | string | `left` | Horizontal alignment when `width > 0`: `left`, `center`, `right`. Invalid values fall back to `left`. |
| `offset` | int | `0` | Horizontal pixel offset of the text. |
| `scale` | number | `1.0` | Scale factor. `>0` and `!=1.0` scales the image. A negative value with `width > 0` auto-scales the image to the widget width (height kept proportional). Resulting dimensions clamp to a minimum of 1px. |
| `inverted` | bool | `0` | Invert pixel colors. |
| `opacity` | number | `1.0` | Overall opacity (0.0–1.0). |
| `center` | bool | `0` | Center the rendered image horizontally across the full display width. |
| `debugborder` | bool | `0` | Draw a 1px border around the canvas for layout debugging. |
| `debugbordercolor` | hex RGB(A) | `ffffff` | Debug border color. If empty or invalid, falls back to `color`. Only drawn when `debugborder` is on. |
| `visible` | bool | `1` | When `0`, the widget renders as transparent. |
| `interval` / `update` | int (ms) | unset | Re-render interval. When `>0`, `reload` is auto-enabled (see Common timing options). |
| `reload` | bool | `0` | Re-render on each update. Auto-managed from `interval` (set to `1` when `interval > 0`, `0` when no interval). |
| `class` | string | — | Accepted but ignored (no effect). |
| `type` | string | `ttf` | Widget type selector; not stored as a property. |

### Shadow

Draws the text offset behind the main text. The canvas is expanded by
`shadowoffset` so the shadow is not clipped.

```
widget:w_shadow {
    type          ttf
    ...
    shadow        1
    shadowcolor   '222222'
    shadowoffset  2
}
```

| Key | Type | Default | Description |
|---|---|---|---|
| `shadow` | bool | `0` | Enable drop shadow. |
| `shadowcolor` | hex RGB(A) | `000000` | Shadow color. Invalid values fall back to `000000`. |
| `shadowoffset` | int | `2` | Shadow displacement in pixels, applied to both x and y. |

### Outline

Draws the text at the 8 surrounding pixel offsets to form a 1px stroke, then
the main text on top. The canvas is expanded by 2px in each dimension.

```
widget:w_outline {
    type         ttf
    ...
    outline      1
    outlinecolor '000000'
}
```

| Key | Type | Default | Description |
|---|---|---|---|
| `outline` | bool | `0` | Enable 1px text outline/stroke. |
| `outlinecolor` | hex RGB(A) | `000000` | Outline color. Invalid values fall back to `000000`. |

Shadow and outline can be combined.


---

## BAR — horizontal/vertical bar graph

Renders a filled bar proportional to a `value` within a `min`–`max` range.
Supports gradients, low/high threshold colors, an optional border, value
smoothing (easing), scaling and centering.

```
widget:w_cpu_bar {
    type      bar
    value     cpu::load()
    min       0
    max       100
    width     460
    height    14
    color     '44cc44'
    bgcolor   '2a2a2a'
    update    1000
    reload    1
}
```

`min`, `max`, `width` (>= 20) and `height` (>= 10) are required; the widget
throws on startup if any is missing or invalid. `value` is conceptually
required but its missing-value check is currently disabled in code, so it
falls back to `min` if omitted.

### Core values

| Key | Type | Default | Description |
|---|---|---|---|
| `value` | int (expression) | `min` | Current value; clamped to `[min, max]`. |
| `min` | int | required | Minimum value. Forced into range 0–210 (out-of-range reset to 0). |
| `max` | int | required | Maximum value. Forced into range 0–250 (out-of-range reset to 100); must be >= `min`. |
| `width` | int | required | Bar width in pixels; minimum 20. |
| `height` | int | required | Bar height in pixels; minimum 10. |
| `direction` | string | `east` | Fill direction. Accepts `north`, `east`, `south`, `west` plus aliases `up`→north, `right`→east, `down`→south, `left`→west. Invalid/missing → `east`. |
| `hollow` | bool | `0` | Draw only the bar outline instead of a solid fill. |

### Colors

| Key | Type | Default | Description |
|---|---|---|---|
| `color` | hex color | `ffffff` | Bar fill color. Reset to `ffffff` if missing/invalid. |
| `bgcolor` | hex color | `000000` | Background fill color. Reset to `000000` if missing/invalid. |
| `colorend` | hex color | — (none) | Gradient end color. When set and valid, the fill is a gradient from `color` to `colorend`. Threshold colors (low/high) override the gradient. |
| `border` | bool | `0` | Draw a 1-pixel border around the full bar area. |
| `bordercolor` | hex color | falls back to `color` | Border color; defaults to the active bar color when `border` is on and this is empty/invalid. |

### Threshold colors

Low/high thresholds are disabled (value `-1`) by default. They are also
auto-disabled if out of the `[min, max]` range, if `high < low`, or if
`low == high`. When the value crosses a threshold the corresponding color
replaces the fill (and disables any gradient).

| Key | Type | Default | Description |
|---|---|---|---|
| `low` | int | `-1` (disabled) | Value threshold; when `value <= low` the bar uses `colorlow`. |
| `high` | int | `-1` (disabled) | Value threshold; when `value >= high` the bar uses `colorhigh`. |
| `colorlow` | hex color | `000000` | Fill color when `value <= low`. Forced to `000000` when `low` is disabled. |
| `colorhigh` | hex color | `000000` | Fill color when `value >= high`. Forced to `000000` when `high` is disabled. |

### Animation, geometry and compositing

| Key | Type | Default | Description |
|---|---|---|---|
| `smooth` | int | `0` | Value easing: the maximum amount the displayed value may step toward the real value per update. NOT anti-aliasing. Auto-disabled if `>= max` or `>= (max - min)`. |
| `scale` | double | `1.0` | Scale factor for the rendered bitmap. A negative value auto-scales width to the widget/display width, keeping height proportional. |
| `center` | bool | `0` | Center the entire rendered widget horizontally within the display width. |
| `inverted` | bool | `0` | Invert pixel colors (not the fill direction). |
| `opacity` | double | `1.0` | Overall per-pixel opacity. |
| `visible` | bool | `1` | When false, the widget renders transparent / is cleared. |

### Timing

| Key | Type | Default | Description |
|---|---|---|---|
| `interval` / `update` | int (ms) | see Common timing | Re-evaluation interval; `update` is an alias of `interval`. If `interval > 0` and no `reload` is set, `reload` defaults to `1`. |
| `reload` | bool | `0` | Re-render on each update interval. |

### Ignored keys

`type` (must be `bar`) and `class` are accepted in the config but are not
stored as widget properties.

**Gradient example:**
```
color     '44cc44'
colorend  ffcc00
```

**Threshold colors:**
```
colorhigh ff4444
high      80
```

Threshold colors take priority over the gradient when the threshold is active.

---

## LINECHART — scrolling line graph

Plots a scrolling history of a value as a vertical-bar line graph. Each render shifts the internal ring buffer left by one and appends the newest sample on the right, so the chart scrolls right-to-left. For every column an upper segment is drawn in `fgcolor` at the sample height and a solid lower segment is drawn in `fgcolor2` from below the line down to the bottom edge.

```
widget:w_cpu_line {
    type      linechart
    value     cpu::load()
    min       0
    max       100
    width     460
    height    50
    fgcolor   '44cc44'
    fgcolor2  '1a441a'
    bgcolor   111111
    gridlines 4
    smooth    20
    update    1000
    reload    1
}
```

### Data and range

| Key | Type | Default | Description |
|---|---|---|---|
| `value` | int (expression) | `min` | Value to plot. Read each update and clamped to `[min, max]`. Effectively optional — the missing-value check is disabled in code, so it defaults to `min`. |
| `min` | int | `0` | Y-axis minimum. **Required** (widget fails to initialize if absent). Validated to range `0–210`; out-of-range values are reset to `0`. |
| `max` | int | `100` | Y-axis maximum. **Required** (widget fails to initialize if absent). Validated to range `0–250`; out-of-range resets to `100`; if `max < min` it becomes `min + 39`. |

### Geometry

| Key | Type | Default | Description |
|---|---|---|---|
| `width` | int | — (required) | Canvas width in pixels. **Required**; must be ≥ 20 or the widget fails to initialize. Also doubles as the history length (one sample per pixel column). |
| `height` | int | — (required) | Canvas height in pixels. **Required**; must be ≥ 10. |
| `scale` | double | `1.0` | Resize factor applied after drawing. `>0` and `≠1` scales both axes by the factor. A **negative** value auto-scales the image width to the widget width, keeping height proportional. |
| `center` | bool | `0` | When `1`, widens the canvas to the full display width and copies the chart into the horizontal center (centers the widget on the display). |

### Colors

| Key | Type | Default | Description |
|---|---|---|---|
| `fgcolor` | hex color | `ffffff` | Line color (the top segment of each column). Invalid/missing values are reset to `ffffff` with a logged error. |
| `fgcolor2` | hex color | `aaaaaa` | Solid fill color for the area beneath the line. Drawn as a vertical line from just under the data point down to the bottom edge. **Not** a gradient. Invalid/missing values reset to `aaaaaa`. |
| `bgcolor` | hex color | `444444` | Background fill color. Invalid/missing values reset to `444444`. |
| `gridcolor` | hex color | auto | Horizontal gridline color. When empty or invalid, gridlines use a dim blend of fg and bg (`fg/4 + bg*3/4`). Only used when `gridlines > 1`. |

### Gridlines and smoothing

| Key | Type | Default | Description |
|---|---|---|---|
| `gridlines` | int | `0` | Number of horizontal grid divisions. Lines are drawn only when `> 1`, producing `gridlines − 1` evenly spaced interior horizontal lines (drawn behind the data). `0` or `1` = no lines. Negative values are clamped to `0`. |
| `smooth` | int | `0` | Smoothing strength, clamped to `0–85`. Uses a heuristic spike-damping smoother that smooths large jumps more aggressively than small drifts (not a true EMA — see curvechart for that). `0` = raw values. |

### Appearance and update behavior

| Key | Type | Default | Description |
|---|---|---|---|
| `opacity` | double | `1.0` | Overall opacity, applied per pixel. |
| `inverted` | bool | `0` | Inverts pixel **colors** (not the axis). |
| `visible` | bool | `1` | When `0`, the widget renders fully transparent. |
| `use_cycles` | int (0/1) | `0` | When `1`, the widget advances by render cycles (using `interval` as the cycle count) instead of wall-clock time. Any value other than `1` is forced to `0`. Aliases: `in_cycles`, `incycles`, `usecycles`, `cycles`, `cycle_interval`. |
| `interval` / `update` | int (ms) | timing default | How often the widget re-evaluates. If `interval > 0` and `reload` is unset, `reload` is forced to `1`. |
| `reload` | bool | `0` | Re-render on every update. Auto-managed in relation to `interval` (see above). |

`class` and `type` are accepted keys but are consumed internally and not stored as widget properties (`type` selects the widget; `class` is reserved).


---

## CURVECHART — smooth scrolling curve graph

Plots a scrolling history of a value as a smooth curve. Each update pushes the
current `value` into a ring buffer of `samples` points; the buffer is drawn
across the full `width` using a Catmull-Rom spline (always applied). When
`samples` < `width`, each sample spans several pixels and produces a visibly
smooth wave; when `samples` == `width`, the mapping is 1:1 and the curve follows
the raw data points.

```
widget:w_cpu_curve {
    type      curvechart
    value     cpu::load()
    min       0
    max       100
    samples   60
    width     460
    height    50
    fgcolor   '44cc44'
    fgcolor2  '1a441a'
    bgcolor   111111
    fill      1
    smooth    20
    update    1000
    reload    1
}
```

`min`, `max`, `width`, and `height` are **required** — the widget fails to
initialise (throws) if any is missing or invalid.

### Data / range

| Key | Type | Default | Description |
|---|---|---|---|
| `value` | int (expression) | `0` | Value to plot; clamped to `[min, max]` |
| `min` | int | **required** | Y-axis minimum. Must be 0-210; an out-of-range value is reset to 0 |
| `max` | int | **required** | Y-axis maximum. Must be 0-250; out-of-range resets to 100; if `max` < `min` it is reset to `min + 39` |
| `samples` | int | `width` | Number of historical data points kept in the ring buffer. Must be 4-4096 and is capped at `width`; out-of-range falls back to `width`. Fewer samples than `width` produces a smoother spline |

### Geometry

| Key | Type | Default | Description |
|---|---|---|---|
| `width` | int | **required** | Canvas width in pixels (minimum 10) |
| `height` | int | **required** | Canvas height in pixels (minimum 4) |
| `scale` | double | `1.0` | Scale factor applied to the rendered image; values < 0 are treated as `1.0` |
| `center` | bool | `0` | Re-canvas to the full display width and horizontally center the chart |

### Colors / appearance

| Key | Type | Default | Description |
|---|---|---|---|
| `fgcolor` | hex color | `44cc44` | Curve line color (invalid/missing resets to `44cc44`) |
| `fgcolor2` | hex color | = `fgcolor` | Fill color beneath the curve; if unset/invalid, falls back to `fgcolor` drawn at reduced opacity |
| `bgcolor` | hex color | `1a1a1a` | Background color (invalid/missing resets to `1a1a1a`) |
| `fill` | bool | `1` (on) | Fill the area beneath the curve with `fgcolor2`. **Enabled by default** |
| `linewidth` | int | `1` | Curve line thickness in pixels, clamped to 1-3. `1` uses anti-aliased drawing |
| `gridlines` | int | `0` | Horizontal grid lines. Only drawn when > 1; draws `gridlines - 1` interior lines |
| `gridcolor` | hex color | auto | Grid line color; defaults to a dim blend of `fgcolor` (¼) and `bgcolor` (¾) |
| `smooth` | int | `0` | Exponential-moving-average smoothing strength for incoming values, clamped 0-85 (0 = none, 85 = heavy). Does **not** toggle the spline — the Catmull-Rom curve is always drawn |
| `inverted` | bool | `0` | Invert pixel colors |
| `opacity` | double | `1.0` | Overall opacity |
| `visible` | bool | `1` | When `0`, the widget renders transparent (and clears its bitmap) |

### Timing

| Key | Type | Default | Description |
|---|---|---|---|
| `interval` / `update` | int (ms) | timing default | How often the value expression is re-evaluated. If `interval` > 0 and no `reload` is given, `reload` is forced to `1` |
| `reload` | bool | `0` | Re-render on each update. Set to `0` automatically if specified without an `interval` |
| `use_cycles` | bool | `0` | Update by render cycles instead of wall-clock time. Aliases: `in_cycles`, `incycles`, `usecycles`, `cycles`, `cycle_interval`. Any value other than `1` is treated as `0` |

### Block keys

| Key | Description |
|---|---|
| `type` | Must be `curvechart` (selects the widget) |
| `class` | Accepted but ignored by this widget |

No plugin functions are registered by this widget (no `CONFIG::functions.append`).


---

## GAUGE — circular arc gauge

Renders a circular dial gauge (ring/donut), similar to a speedometer or VU meter. A background "track" arc is drawn for the full sweep, the "value" arc is filled on top proportional to `value`, and the center is hollowed out to leave a ring. An optional needle can be drawn inside the hollow face.

```
widget:w_cpu_gauge {
    type        gauge
    value       cpu::load()
    min         0
    max         100
    width       120
    height      120
    fgcolor     '44cc44'
    bgcolor     '111111'
    linewidth   16
    startangle  225
    sweepangle  270
    update      1000
    reload      1
}
```

`width` and `height` are **required** (minimum 20px each); the widget fails to initialise if either is missing or smaller than 20.

### Geometry and value

| Key | Type | Default | Description |
|---|---|---|---|
| `value` | int / expression | `min` | Current value (expression). Clamped to `[min, max]`. If omitted the gauge sits at `min`. |
| `min` | int | `0` | Minimum value (gauge empty). |
| `max` | int | `100` | Maximum value (gauge full). If `max <= min` it is forced to `min + 100` (an error is logged). |
| `width` | int | — (required) | Canvas width in px. Required; must be ≥ 20 or init fails. |
| `height` | int | — (required) | Canvas height in px. Required; must be ≥ 20 or init fails. |
| `linewidth` | int | `max(4, min(width,height)/7)` | Arc/ring stroke width in px. Clamped at render to `[2, radius-1]` where `radius = min(width,height)/2`. |
| `startangle` | int (deg) | `225` | Arc start angle. 0° = 3 o'clock, increasing clockwise. |
| `sweepangle` | int (deg) | `270` | Total arc sweep. Clamped to `[10, 360]`. |

**Angle reference:** 0° = 3 o'clock, increasing clockwise. The default `startangle=225` places the start at about 7–8 o'clock and `sweepangle=270` sweeps clockwise nearly all the way around to about 4–5 o'clock.

### Colors

| Key | Type | Default | Description |
|---|---|---|---|
| `fgcolor` | hex color | `'44cc44'` | Active (value) arc color. If missing/empty/invalid it is forced to `'44cc44'` (error logged). |
| `bgcolor` | hex color | `'111111'` | Face/background color (also fills the hollow center). If missing/empty/invalid it is forced to `'111111'` (error logged). |
| `trackcolor` | hex color | auto | Background track arc color. If unset/invalid, defaults to a dim blend of ¼ `fgcolor` + ¾ `bgcolor` per channel. |

### Threshold colors

The active arc color can switch based on the value:

| Key | Type | Default | Description |
|---|---|---|---|
| `low` | int | `-1` | Low threshold. Disabled when `< 0`. When `value <= low`, the arc uses `colorlow`. |
| `high` | int | `-1` | High threshold. Disabled when `< 0`. When `value >= high`, the arc uses `colorhigh`. |
| `colorlow` | hex color | `'000000'` | Arc color when `value <= low`. `'000000'` is the disabled sentinel — the low color only takes effect when set to another valid color. |
| `colorhigh` | hex color | `'000000'` | Arc color when `value >= high`. `'000000'` is the disabled sentinel. |

The low check is evaluated first; if it does not apply, the high check is evaluated. When neither threshold is active the arc uses `fgcolor`.

### Needle

| Key | Type | Default | Description |
|---|---|---|---|
| `needle` | bool | `0` | When `1`, draws a needle line from the center plus a center hub dot, on top of the ring (the arc is still drawn). Only rendered when the inner radius `> 2`. |
| `needlecolor` | hex color | active arc color | Needle/hub color. If unset/invalid the needle uses the current active arc color (`fgcolor`, or the active threshold color). |

### Visibility and timing

| Key | Type | Default | Description |
|---|---|---|---|
| `visible` | bool | `true` | When false the gauge is blanked/cleared instead of rendered. |
| `interval` / `update` | int (ms) | — | Re-evaluation interval. `update` is an alias for `interval`. If `interval > 0` and `reload` is unset, `reload` is forced to `1`. |
| `reload` | bool | `0` | Re-render on each update. With `reload 0` the gauge only re-renders when the (clamped) value changes. |

> Notes: `class` is accepted but ignored. `type` must be `gauge` and is consumed by the loader (not stored as a property). The gauge registers no plugin functions.


---

## CLOCK — analog clock

Renders a circular analog clock face showing the current local system time. The
clock reads `std::localtime` directly and does **not** use a `value` expression.

```
widget:w_clock {
    type        clock
    width       120
    height      120
    facecolor   '1a1a2a'
    rimcolor    '666688'
    hourcolor   ffffff
    minutecolor cccccc
    secondcolor ff4444
    tickcolor   '888888'
    ticks       1
    minuteticks 0
    handwidth   0
    update      1000
    reload      1
}
```

`width` and `height` are **required**; the widget fails to initialise if either
is missing or below `20`.

### Geometry & colors

| Key | Type | Default | Description |
|---|---|---|---|
| `width` | int (px) | — (required, min 20) | Canvas width. Must be ≥ 20 or the widget throws at init. |
| `height` | int (px) | — (required, min 20) | Canvas height. Must be ≥ 20 or the widget throws at init. |
| `facecolor` | color | `1a1a2a` | Inner clock-face disc color. Invalid values fall back to `1a1a2a`. |
| `rimcolor` | color | `666688` | Outer rim/border ring color. Invalid values fall back to `666688`. |
| `hourcolor` | color | `ffffff` | Hour hand color (also used for the center hub dot). |
| `minutecolor` | color | `cccccc` | Minute hand color. |
| `secondcolor` | color | `ff4444` | Second hand color (always 1px thick). |
| `tickcolor` | color | `888888` | Color of both hour and minute tick marks. |
| `ticks` | bool | `1` (true) | Draw the 12 hour tick marks (longer, 2px). |
| `minuteticks` | bool | `0` (false) | Draw the 60 minute tick marks (shorter, 1px). Enabling this also forces hour ticks to draw even if `ticks` is `0`. |
| `handwidth` | int (px) | `0` | Hand stroke width, clamped to `0`–`8`. `0` = auto-size from canvas: hour hand `max(2, diameter/20)`, minute hand `max(1, diameter/30)`. When > 0: hour hand = `handwidth`, minute hand = `max(1, handwidth-1)`. |

Colors accept the same hex formats as other widgets (e.g. `ffffff`, `'1a1a2a'`,
optionally with alpha). Any value failing color validation silently reverts to
the default shown above.

> **Note:** `bgcolor` is accepted in the config without warning but is currently
> **unused** — the canvas background is always transparent (only the circular
> face and rim are drawn; the corners outside the circle stay transparent).
> Setting `bgcolor` has no visible effect.

### Timing

| Key | Type | Default | Description |
|---|---|---|---|
| `interval` / `update` | int (ms) | unset | Re-render interval. `update` is an alias rewritten to `interval`. If `interval > 0` and `reload` is unset, `reload` is auto-set to `1`. |
| `reload` | bool | `0` | Re-render every cycle. A clock needs `reload 1` (directly or via a positive `interval`) to keep ticking. If `reload` is set but no `interval` is given, `interval` is forced to `0`. |
| `visible` | bool | `1` (true) | When false the clock is hidden and its bitmap is cleared/freed. |

### Other accepted keys

| Key | Description |
|---|---|
| `type` | Widget type selector; must be `clock`. Consumed during parsing. |
| `class` | Accepted but consumed and discarded (no effect on rendering). |

### Rendering notes

- The clock is drawn into a square inscribed circle of `diameter = min(width, height)`,
  centered in the canvas; non-square canvases leave empty space on the longer axis.
- Rim thickness scales with size as `max(2, diameter/40)`.
- The second hand is always 1px; the center hub dot uses `hourcolor` sized `max(3, diameter/20)`.
- Hands are drawn anti-aliased.


---

## IMAGE — static or dynamic image

Renders an image file onto the display. Supported formats are **PNG, JPEG, GIF and BMP** — the loader tries each decoder in that order (`render()`, image.cpp lines 148-176). The image can be resized (by fixed width/height, by a scale factor, or auto-scaled to the widget width) and optionally centered on the display.

```
widget:w_logo {
    type    image
    file    '/usr/share/icons/logo.png'
    width   64
    height  64
}
```

A "dynamic" image is achieved by enabling periodic reloads: set `update`/`interval`, and the widget re-reads the *same file path* from disk on each cycle. The `file` value is a literal path — it is **not** an expression and cannot contain a function call.

### Options

| Key | Type | Default | Description |
|---|---|---|---|
| `file` | string (path) | — (**required**) | Path to the image file (PNG/JPEG/GIF/BMP). Read as a literal path. Construction fails if missing or empty. |
| `width` | int (px) | `0` | Target width. `0` = no width-based resize. Combined with `height` for aspect-preserving fit; also used as the target width when `scale` is negative. |
| `height` | int (px) | `0` | Target height. `0` = no height-based resize. Combined with `width` for aspect-preserving fit. |
| `scale` | number | `1.0` | Scale factor. `>0` and `!=1.0` scales by that factor. `<0` (with `width>0`) auto-scales to `width`, preserving aspect ratio. `1.0` or `0` leaves the width/height fit path active. |
| `center` | bool | `0` | Horizontally center the image on a canvas spanning the full display width. |
| `opacity` | number | `1.0` | Per-pixel alpha multiplier (`0.0`–`1.0`). |
| `inverted` | bool | `0` | Invert pixel colors. |
| `visible` | bool | `1` | When `0`, the widget renders transparent (and clears its bitmap). |
| `reload` | bool | `0` | Re-render (re-read the file) on each update cycle. Auto-managed — see below. |
| `interval` / `update` | int (ms) | `1500` (min `50`) when reloading; `0` otherwise | Re-read interval. `update` is an alias for `interval`. |

### Resize behavior

- **Fit to box** — when `width>0` or `height>0` and `scale` is `1.0`/`0`, the image is scaled to fit inside the `width × height` box while preserving aspect ratio (lines 185-219).
- **Scale factor** — when `scale>0` and `scale!=1.0`, the image is multiplied by `scale` (lines 221-248).
- **Auto-scale to width** — when `scale<0` and `width>0`, the image is scaled so its width equals `width`, with height derived from the original aspect ratio (lines 229-232).

### Reload / interval coupling

The image widget couples `reload` and `interval` at construction (lines 53-59):

- If `interval` (or `update`) `> 0` is given but `reload` is not set, `reload` is automatically enabled (`1`).
- If `reload` is set but no `interval` is given, `reload` is forced back to `0` (a one-shot render).
- When reloading, `interval` defaults to `1500` ms and is clamped to a minimum of `50` ms.

So a static image needs neither key; a periodically-refreshing image only needs `update <ms>`.

> Note: `type` is the required widget selector (`type image`) and `class` is accepted but ignored.


---

## Common options

Every widget accepts these in addition to its type-specific keys:

| Key | Type | Default | Description |
|---|---|---|---|
| `type` | string | — (required) | Widget type selector; consumed by the loader, not stored as a property. |
| `class` | string | — | Accepted for forward-compatibility but currently ignored by all widgets. |
| `visible` | bool | `1` | When `0`, the widget renders fully transparent (its bitmap is cleared). |
| `interval` / `update` | int (ms) | `1500` (min `50`) | How often the widget re-evaluates its value/text expression. `update` is an alias for `interval`. |
| `reload` | bool | `0` | Re-render on every update cycle. Auto-coupled to `interval`: a positive `interval` with no explicit `reload` sets `reload 1`; a `reload` with no `interval` is forced to `0` (one-shot). |

A widget with `reload 0` renders once (or only when its value changes). Use
`reload 1` — or simply a positive `update` — for widgets whose appearance must
refresh continuously (scrolling charts, the clock, periodically-reloaded images).
