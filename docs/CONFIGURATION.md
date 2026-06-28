# Configuration Reference

lcd2 reads a single configuration file (default: `lcd2.conf`). Pass a different file with the `-c` flag:

```sh
./lcd2 -c /etc/lcd2.conf
```

## Syntax

The config format uses named blocks (`name { ... }`) and key-value pairs (`key value`).

```
# This is a comment — only '#' comments are supported

SectionName {
    key   value
    key2  'value with spaces'
    key3  expression()
}
```

Only `#` starts a comment (anywhere on a line). `//` and `/* */` are **not**
recognised — a line beginning with them is parsed as a (garbage) section/key name.

Arrays use bracket syntax, one value per line or separated by commas/spaces:

```
timers [
    rotate
    blink
]
```

### Value quoting rules

| Value type | Rule | Example |
|---|---|---|
| Simple words | Unquoted | `drm`, `left`, `ffffff` |
| Paths | **Must** be quoted | `'/usr/share/fonts/Font.ttf'` |
| Hex colors starting with a digit | **Must** be quoted | `'44cc44'`, `'0088ff'` |
| All-letter hex colors | Unquoted | `ffffff`, `aabbcc` |
| Expressions | Always unquoted | `cpu::load()` |
| Numbers | Always unquoted | `1000`, `0`, `100` |

A bare value that is a single all-alphanumeric word is auto-treated as a string
literal (this is why `drm` and `ffffff` work unquoted). A digit-leading token such
as `44cc44` is first seen as a number, so it must be quoted to be used as a color.

---

## Display block

Exactly one `display { }` block is required.

```
display {
    driver          drm
    device          '/dev/dri/card0'
    orientation     0               # 0/1/2/3 = 0°/90°/180°/270°
    backlight       5               # 0–10 level (not a percentage)
    backlight_path  auto            # DRM only: auto | disabled | explicit sysfs path
}
```

The display **resolution is not configurable** — it is always determined by the
driver (DRM reads the connector's preferred mode; DPF queries the panel over USB).
There are no `width`/`height` keys.

These are the only accepted keys (anything else is rejected as unsupported):

| Key | Type | Default | Description |
|---|---|---|---|
| `driver` | `drm` \| `dpf` | — (required) | Display driver. Unknown/empty driver aborts startup. |
| `device` | string | *(empty)* | Output device. DRM: DRI node, defaults to `/dev/dri/card0`. DPF: **required**, a 4-char id like `usb0`…`usb9` (or `dpf0`…). |
| `orientation` | `0`–`3` | `0` | Rotation: 0°, 90°, 180°, 270°. Out-of-range values are ignored with a warning. |
| `foreground` | hex color | `ffffff` | Default foreground color (`RGBA::FG`). |
| `background` | hex color | `000000` | Default background color (`RGBA::BG`). |
| `basecolor` | hex color | `000000` | Base/clear color blended under transparent layers (`RGBA::BL`). |
| `backlight` | `0`–`10` | `5` | Backlight level on a **0–10** scale (not 0–100). Out-of-range warns and resets to 5; the driver maps it onto the panel range (DPF clamps to 0–7). |
| `backlight_path` | `auto` \| `disabled` \| path | `auto` | **DRM only** (DPF ignores it). `auto` scans `/sys/class/backlight`; `disabled` = no control; an explicit sysfs dir uses its `max_brightness`. |

Both drivers also register two equivalent expression/action functions,
`backlight()` and `brightness()`, that get or set the backlight level at runtime.

> **DPF/AX206 is deprecated.** A DRM/KMS driver for the same hardware exists and
> works better — prefer `driver drm`. The `dpf` driver is retained for reference.

---

## Widget blocks

Each widget is defined with `widget:name { }`.

```
widget:my_text {
    type    ttf
    font    '/usr/share/fonts/MyFont.ttf'
    size    16
    color   ffffff
    text    uname::hostname()
    update  60000
}
```

Keys available on **all** widget types:

| Key | Default | Description |
|---|---|---|
| `type` | — | Widget type (required): `ttf`, `bar`, `linechart`, `curvechart`, `image`, `gauge`, `clock` |
| `visible` | `1` | Show/hide: `0` or `1`, or an expression |
| `interval` / `update` | `1500` | Value re-evaluation interval in milliseconds (`update` is an alias) |
| `reload` | `0` | Re-render on every update when `1`; auto-coupled to `interval` |

Other keys (`width`, `height`, `scale`, `center`, `opacity`, `inverted`, colors,
…) are **type-specific** with per-widget defaults and semantics — for example
`width`/`height` are required for `bar`/`gauge`/`clock`/`curvechart` but auto-sized
for `ttf`. See [WIDGETS.md](WIDGETS.md) for the exact options of each type.

---

## Layout block

The `layout { }` block positions widgets on screen and organises them into pages.

```
layout {
    default 0       # starting page number
    sequence 0,1    # page order for page::next / page::prev (optional)

    timers [        # timers active on every page (optional)
        rotate
    ]

    page:0 {
        widget_name   x,y
        widget_name   x,y
    }

    page:1 {
        widget_name   x,y
    }
}
```

Coordinates are `x,y` in pixels from the top-left corner.

### Page sequence

`sequence` defines the order in which `page::next()` and `page::prev()` walk the
pages. It is a list of page numbers separated by spaces or commas (ranges with `-`
are **not** supported — list each page):

```
sequence 0,1,2      # next() cycles 0 → 1 → 2 → 0 …
```

Without a `sequence`, `page::next()`/`page::prev()` have nothing to walk and log
`layout page sequence is empty`. Only existing pages are kept; negatives are made
positive and consecutive duplicates dropped. The sequence needs **at least 2
distinct existing pages** or it is discarded entirely.

### Page handlers and goodbye

Each `page:N { }` may name a timer to fire once on entering or leaving it, and the
layout may define a special `goodbye { }` page rendered on shutdown:

```
layout {
    page:0 {
        on_enter blink     # fire timer 'blink' once when this page becomes active
        on_exit  fade       # fire timer 'fade' once when leaving this page
        timers [ ticker ]   # timers that run only while page 0 is active
        w_clock 10,10
    }

    goodbye {               # shown on exit (internal page -1); no 'timers' key allowed
        w_bye 10,10
    }
}
```

`on_enter` / `on_exit` reference a timer by name; if the named timer does not
exist the handler is cleared with a logged error. Page-level `timers [ ]` run only
while that page is active (names already registered globally are stripped).

### Layers

Widgets within a page can be grouped into layers. Layer 0 is drawn first (bottom), higher layers on top.

```
page:0 {
    layer:0 {
        background_widget   0,0
    }
    layer:1 {
        foreground_widget   10,10
    }
}
```

Widgets placed directly inside a `page` block (without an explicit `layer`) are automatically assigned to layer 0.

---

## Plugin configuration (optional)

Most plugins need no configuration and are always available. The only plugin currently supporting optional configuration is `ubus` (when built with `WITH_UBUS=1`):

```
plugin:ubus {
    socket  '/var/run/ubus/ubus.sock'
}
```

See [UBUS.md](UBUS.md) for details.

---

## Timers

A `timer:` block runs an action and/or evaluates expressions on a fixed interval.
A timer only runs while it is **registered** — either globally via the layout's
`timers [ ]` list, per page via `page:N { timers [ ... ] }`, or as a page's
`on_enter` / `on_exit` handler.

```
timer:rotate {
    interval 3000          # ms between runs (alias: update); clamped to 50–86400000
    action   page::next()  # action to run each interval (see Actions)
    # condition cpu::load() > 80   # optional: only run the action when non-zero (true)
    # active 1              # optional: set 0 to disable the timer
}
```

Recognised keys: `interval` (or `update`), `action`, `condition`, `active`, and
one or more `expression` entries.

- Without a `condition`, the `action` runs every interval.
- A `condition` is only honoured together with an `action`; on its own it does
  nothing. When set, the `action` runs only on intervals where the condition
  evaluates to non-zero (true).
- A timer defined with neither an `action` nor any `expression` fails to
  initialise (it is rejected at startup, not silently kept).

Register the timer so it actually runs:

```
layout {
    timers [ rotate ]      # global: runs on every page
    page:0 {
        timers [ blink ]   # per page: runs only while page 0 is the active page
    }
}
```

---

## Actions

Actions are invoked from a timer's `action` key (or a page `on_enter` / `on_exit`
timer) using **function-call syntax** — note the parentheses, which are required:

| Call | Effect |
|---|---|
| `page::next()` | switch to the next page in the layout `sequence` |
| `page::prev()` | switch to the previous page in the `sequence` |
| `page::set(N)` | switch to page number `N` |
| `log('message'[, ...])` | write a message to the log; multiple arguments are joined with spaces, numbers stringified |

`page::next()` / `page::prev()` require a `sequence` to be defined in the layout
(see [Page sequence](#page-sequence)).

### Page rotation (complete example)

Cycle through pages 0 and 1 every 3 seconds — note the three required pieces:
a `timer:` with the action, the `timers [ ]` registration, and the `sequence`.

```
layout {
    default  0
    sequence 0,1           # 1. rotation order
    timers [ rotate ]      # 2. activate the timer

    page:0 { w_clock 10,10 }
    page:1 { w_info  10,10 }
}

timer:rotate {             # 3. the timer that drives the rotation
    interval 3000
    action   page::next()
}
```

---

## Expression engine

Widget values and labels are evaluated as expressions. Expressions support:

- **String literals:** `'hello world'`
- **Number literals:** `42`, `3.14`
- **String concatenation:** `'CPU: ' . format(cpu::load(), 1) . ' %'`
- **Arithmetic:** `mem::used('mb') / 1024`
- **Ternary/conditional:** `cpu::load() > 80 ? 'HOT' : 'OK'`
- **Function calls:** `cpu::load()`, `strftime('%H:%M', time())`, `format(value, decimals)`

See [PLUGINS.md](PLUGINS.md) for all plugin functions and the full list of
built-in expression functions (math, string, time/date, type-conversion).
