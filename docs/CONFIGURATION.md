# Configuration Reference

lcd2 reads a single configuration file (default: `lcd2.conf`). Pass a different file with the `-c` flag:

```sh
./lcd2 -c /etc/lcd2.conf
```

## Syntax

The config format uses named blocks (`name { ... }`) and key-value pairs (`key value`).

```
# This is a comment
// This is also a comment
/* Block
   comment */

SectionName {
    key   value
    key2  'value with spaces'
    key3  expression()
}
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

---

## Display block

Exactly one `display { }` block is required.

```
display {
    driver          drm
    device          '/dev/dri/card0'
    width           1920            # optional: auto-detected for DRM
    height          1080            # optional: auto-detected for DRM
    backlight       80              # optional: 0–100
    backlight_path  auto            # auto | disabled | explicit sysfs path
}
```

| Key | Values | Description |
|---|---|---|
| `driver` | `drm`, `dpf` | Display driver to use |
| `device` | path | Device node, e.g. `/dev/dri/card0` |
| `width` | integer | Display width in pixels (DRM: auto-detected) |
| `height` | integer | Display height in pixels (DRM: auto-detected) |
| `backlight` | 0–100 | Backlight brightness percentage |
| `backlight_path` | `auto` / `disabled` / path | Sysfs backlight control path |

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

Common keys available on all widgets:

| Key | Default | Description |
|---|---|---|
| `type` | — | Widget type (required): `ttf`, `bar`, `linechart`, `curvechart`, `image`, `gauge`, `clock` |
| `width` | — | Width in pixels |
| `height` | — | Height in pixels |
| `visible` | `1` | Show/hide: `0` or `1`, or an expression |
| `interval` / `update` | `1500` | Refresh interval in milliseconds |
| `reload` | `0` | Re-evaluate the layout on each update if `1` |
| `opacity` | `1.0` | Overall opacity (0.0–1.0) |
| `inverted` | `0` | Invert colors |
| `scale` | `1` | Scale factor |
| `center` | `0` | Center the widget content |

See [WIDGETS.md](WIDGETS.md) for per-type options.

---

## Layout block

The `layout { }` block positions widgets on screen and organises them into pages.

```
layout {
    default 0       # starting page number

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
Plugin ubus {
    socket  '/var/run/ubus/ubus.sock'
}
```

See [UBUS.md](UBUS.md) for details.

---

## Actions

Actions are triggered by events (currently page change events).

```
action:my_action {
    type    log
    message 'Switched to page'
}
```

Available action types: `log`, `setpage`, `nextpage`, `prevpage`.

---

## Expression engine

Widget values and labels are evaluated as expressions. Expressions support:

- **String literals:** `'hello world'`
- **Number literals:** `42`, `3.14`
- **String concatenation:** `'CPU: ' . format(cpu::load(), 1) . ' %'`
- **Arithmetic:** `mem::used('mb') / 1024`
- **Ternary/conditional:** `cpu::load() > 80 ? 'HOT' : 'OK'`
- **Function calls:** `cpu::load()`, `strftime('%H:%M', time())`, `format(value, decimals)`

See [PLUGINS.md](PLUGINS.md) for all available functions.
