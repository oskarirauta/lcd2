# Widget Reference

Widgets are defined in the config file as `widget:name { }` blocks. Every widget requires a `type` key.

---

## TTF — TrueType text

Renders a text string using a TrueType font.

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

| Key | Default | Description |
|---|---|---|
| `text` | — | Text to render (expression or string literal) |
| `font` | — | Path to a `.ttf` font file (required) |
| `size` | `12.0` | Font size in points |
| `color` | `ffffff` | Text color (hex RGB) |
| `width` | auto | Canvas width; auto-sized to text if omitted |
| `height` | auto | Canvas height; auto-sized to text if omitted |
| `align` | `left` | Text alignment: `left`, `center`, `right` |
| `offset` | `0` | Horizontal pixel offset |
| `opacity` | `1.0` | Overall opacity |
| `inverted` | `0` | Invert colors |
| `center` | `0` | Center the text in the canvas |
| `scale` | `1` | Scale factor |
| `debugborder` | `0` | Draw a debug border around the canvas |
| `debugbordercolor` | — | Debug border color |

### Shadow

```
widget:w_shadow {
    type          ttf
    ...
    shadow        1
    shadowcolor   '222222'
    shadowoffset  2
}
```

| Key | Default | Description |
|---|---|---|
| `shadow` | `0` | Enable drop shadow |
| `shadowcolor` | `444444` | Shadow color (hex RGB) |
| `shadowoffset` | `2` | Shadow offset in pixels (both x and y) |

### Outline

```
widget:w_outline {
    type         ttf
    ...
    outline      1
    outlinecolor '000000'
}
```

| Key | Default | Description |
|---|---|---|
| `outline` | `0` | Enable text outline (stroke) |
| `outlinecolor` | `000000` | Outline color (hex RGB) |

Shadow and outline can be combined.

---

## BAR — horizontal/vertical bar graph

Renders a filled bar proportional to a value within a min–max range.

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

| Key | Default | Description |
|---|---|---|
| `value` | — | Current value (expression, required) |
| `min` | `0` | Minimum value |
| `max` | `100` | Maximum value |
| `color` | — | Bar fill color (hex RGB) |
| `colorend` | — | Gradient end color; creates a gradient from `color` to `colorend` |
| `bgcolor` | — | Background color |
| `direction` | `east` | Fill direction: `east`, `west`, `north`, `south` |
| `low` | — | Value threshold below which `colorlow` is used |
| `high` | — | Value threshold above which `colorhigh` is used |
| `colorlow` | — | Color when value ≤ `low` |
| `colorhigh` | — | Color when value ≥ `high` |
| `hollow` | `0` | Draw only the border of the bar, not the fill |
| `smooth` | `0` | Enable smooth/anti-aliased edges |
| `border` | `0` | Draw a 1-pixel border around the full bar area |
| `bordercolor` | color | Border color (defaults to bar color) |
| `center` | `0` | Center the bar fill from the middle |
| `inverted` | `0` | Invert the fill direction |
| `opacity` | `1.0` | Overall opacity |

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

Threshold colors take priority over gradient when the threshold is active.

---

## LINECHART — scrolling line graph

Plots a scrolling history of a value as a line graph.

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
    update    1000
    reload    1
}
```

| Key | Default | Description |
|---|---|---|
| `value` | — | Value to plot (expression) |
| `min` | `0` | Y-axis minimum |
| `max` | `100` | Y-axis maximum |
| `fgcolor` | — | Line color |
| `fgcolor2` | — | Fill color beneath the line (gradient fill to bgcolor) |
| `bgcolor` | — | Background color |
| `gridlines` | `0` | Number of horizontal grid lines (0 = none; N draws N-1 interior lines) |
| `gridcolor` | auto | Grid line color (defaults to a dim blend of fg and bg) |
| `smooth` | `0` | Smooth the line |
| `center` | `0` | Center the zero line |
| `scale` | `1` | Scale factor |
| `use_cycles` | `0` | Update by render cycles instead of wall-clock time |
| `inverted` | `0` | Invert Y axis |
| `opacity` | `1.0` | Overall opacity |

---

## CURVECHART — smooth scrolling curve graph

Like linechart but draws a smooth Catmull-Rom spline through the data points.

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
    update    1000
    reload    1
}
```

| Key | Default | Description |
|---|---|---|
| `value` | — | Value to plot (expression) |
| `min` | `0` | Y-axis minimum |
| `max` | `100` | Y-axis maximum |
| `samples` | `width` | Number of historical data points to keep |
| `fgcolor` | — | Curve color |
| `fgcolor2` | — | Fill color beneath the curve |
| `bgcolor` | — | Background color |
| `fill` | `0` | Fill area beneath the curve |
| `linewidth` | `1` | Curve line width in pixels |
| `gridlines` | `0` | Number of horizontal grid lines |
| `gridcolor` | auto | Grid line color |
| `smooth` | `1` | Enable Catmull-Rom smoothing (disable for raw polyline) |
| `center` | `0` | Center the zero line |
| `scale` | `1` | Scale factor |
| `use_cycles` | `0` | Update by render cycles instead of wall-clock time |
| `inverted` | `0` | Invert Y axis |
| `opacity` | `1.0` | Overall opacity |

---

## GAUGE — circular arc gauge

Renders a circular dial gauge, similar to a speedometer or VU meter.

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
    startangle  225
    sweepangle  270
    update      1000
    reload      1
}
```

| Key | Default | Description |
|---|---|---|
| `value` | — | Current value (expression) |
| `min` | `0` | Minimum value (gauge empty) |
| `max` | `100` | Maximum value (gauge full) |
| `fgcolor` | — | Active arc color |
| `trackcolor` | auto | Background track arc color (defaults to a dim blend of fg+bg) |
| `bgcolor` | — | Face/background color |
| `linewidth` | ~1/7 of min dimension | Arc stroke width in pixels |
| `startangle` | `225` | Start angle in degrees (0=right/3 o'clock, clockwise) |
| `sweepangle` | `270` | Total arc sweep in degrees |
| `low` | — | Value threshold below which `colorlow` is used |
| `high` | — | Value threshold above which `colorhigh` is used |
| `colorlow` | — | Arc color when value ≤ `low` |
| `colorhigh` | — | Arc color when value ≥ `high` |
| `needle` | `0` | Show a needle pointer instead of (or in addition to) the arc |
| `needlecolor` | `ffffff` | Needle color |

**Angle reference:** 0° = 3 o'clock, increasing clockwise. The default `startangle=225` places the start at about 7 o'clock, and `sweepangle=270` sweeps almost all the way around to 5 o'clock.

---

## CLOCK — analog clock

Renders a circular analog clock face showing the current local time.

```
widget:w_clock {
    type        clock
    width       120
    height      120
    facecolor   '1a1a2e'
    rimcolor    '888888'
    hourcolor   ffffff
    minutecolor cccccc
    secondcolor ff4444
    tickcolor   '666666'
    ticks       1
    update      1000
    reload      1
}
```

| Key | Default | Description |
|---|---|---|
| `facecolor` | `000000` | Clock face background color |
| `rimcolor` | `888888` | Outer rim/border color |
| `bgcolor` | transparent | Widget canvas background (transparent corners outside the circle) |
| `hourcolor` | `ffffff` | Hour hand color |
| `minutecolor` | `cccccc` | Minute hand color |
| `secondcolor` | `ff4444` | Second hand color |
| `tickcolor` | `666666` | Hour tick mark color |
| `ticks` | `1` | Show hour tick marks |
| `minuteticks` | `0` | Show minute tick marks (60 smaller marks) |
| `handwidth` | `2` | Hand stroke width in pixels |

The clock reads the current local system time directly and does not require a `value` expression.

---

## IMAGE — static or dynamic image

Renders an image file (PNG, JPEG, GIF) onto the display.

```
widget:w_logo {
    type    image
    file    '/usr/share/icons/logo.png'
    width   64
    height  64
}
```

| Key | Default | Description |
|---|---|---|
| `file` | — | Path to image file (expression, so can be dynamic) |
| `width` | — | Target width in pixels |
| `height` | — | Target height in pixels |
| `scale` | `1` | Scale factor |
| `center` | `0` | Center the image in the canvas |
| `opacity` | `1.0` | Overall opacity |
| `inverted` | `0` | Invert colors |

---

## Common timing options

| Key | Default | Description |
|---|---|---|
| `interval` / `update` | `1500` | How often (ms) the widget re-evaluates its value expression |
| `reload` | `0` | If `1`, the widget re-renders on every update |

A widget with `reload 0` only renders once (or when its value changes). Use `reload 1` for widgets whose appearance must update on every cycle (e.g. scrolling charts, the clock).
