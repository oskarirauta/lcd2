# ubus Integration

[ubus](https://openwrt.org/docs/techref/ubus) is OpenWrt's IPC bus system. lcd2 can query ubus objects and use the returned JSON data as widget values, so you can display router status, network information, system load, and any other data exposed through ubus — without writing shell scripts.

## Requirements

- OpenWrt, or Alpine Linux with ubus installed (`apk add ubus ubus-dev libubox-dev`)
- ubusd running and accessible via its socket

## Building with ubus support

```sh
make WITH_UBUS=1
```

This compiles in the ubus plugin and the JSON library. Without this flag, neither the `ubus()` function nor any JSON code is included in the binary.

## Socket path configuration (optional)

The default socket path is `/var/run/ubus/ubus.sock`. If your system uses a different path, add a `Plugin ubus` block to your config:

```
Plugin ubus {
    socket  '/var/run/ubus/ubus.sock'
}
```

This block is optional. If omitted, the default path is used.

---

## The `ubus()` expression function

```
ubus(path, method)
ubus(path, method, jsonpath)
ubus(path, method, jsonpath, args)
```

| Argument | Description |
|---|---|
| `path` | ubus object path, e.g. `'system'`, `'network.interface.wan'` |
| `method` | Method name, e.g. `'info'`, `'status'` |
| `jsonpath` | Dot-notation path into the JSON response (optional) |
| `args` | JSON string of method arguments (optional, default empty) |

**Return value:**
- If `jsonpath` is given and resolves to a number → returns a number
- If it resolves to a string → returns a string
- If it resolves to a boolean → returns `1.0` or `0.0`
- If `jsonpath` is omitted → returns the full JSON response as a string
- On error → returns an empty string

Results are cached for 2 seconds, so frequent widget updates do not hammer ubusd.

---

## JSON path navigation

Use dot `.` to navigate objects. Use numeric segments for array indices.

```
'memory.total'       → response["memory"]["total"]
'load.0'             → response["load"][0]
'ipv4-address.0.address'  → response["ipv4-address"][0]["address"]
```

---

## Examples

### system info (OpenWrt)

`ubus call system info` returns:
```json
{
    "localtime": 1779134538,
    "uptime": 3696034,
    "load": [6976, 1888, 416],
    "memory": {
        "total": 33592467456,
        "free": 31769677824,
        "shared": 13189120,
        "buffered": 43712512,
        "available": 32447758336,
        "cached": 875741184
    }
}
```

Config examples:
```
# Memory used percentage
widget:w_mem {
    type    bar
    value   (ubus('system', 'info', 'memory.total') - ubus('system', 'info', 'memory.free')) / ubus('system', 'info', 'memory.total') * 100
    min     0
    max     100
    width   200
    height  16
    color   '4488ff'
    bgcolor '222222'
    update  3000
    reload  1
}

# Load average (1-minute, OpenWrt format is scaled by 65536)
widget:w_load {
    type    ttf
    font    '/usr/share/fonts/MyFont.ttf'
    size    14
    color   ffffff
    text    'Load: ' . format(ubus('system', 'info', 'load.0') / 65536.0, 2)
    update  5000
    reload  1
}

# Uptime in seconds
widget:w_uptime {
    type    ttf
    font    '/usr/share/fonts/MyFont.ttf'
    size    12
    color   888888
    text    'Up: ' . ubus('system', 'info', 'uptime') . 's'
    update  60000
    reload  1
}
```

### Network interface status (OpenWrt)

`ubus call network.interface.wan status` returns interface status including IP address.

```
widget:w_wan_ip {
    type    ttf
    font    '/usr/share/fonts/MyFont.ttf'
    size    14
    color   '44cc44'
    text    'WAN: ' . ubus('network.interface.wan', 'status', 'ipv4-address.0.address')
    update  30000
    reload  1
}
```

### Gauge from ubus value

```
widget:w_mem_gauge {
    type        gauge
    value       (ubus('system', 'info', 'memory.total') - ubus('system', 'info', 'memory.free')) / ubus('system', 'info', 'memory.total') * 100
    min         0
    max         100
    width       100
    height      100
    fgcolor     '4488ff'
    bgcolor     '111111'
    colorhigh   ff4444
    high        85
    update      3000
    reload      1
}
```

### Full JSON response as a string

Omit the `jsonpath` argument to get the raw JSON:

```
widget:w_debug {
    type    ttf
    font    '/usr/share/fonts/MyFont.ttf'
    size    10
    color   888888
    text    ubus('system', 'info')
    update  5000
    reload  1
}
```

### Method with arguments

Some ubus methods accept arguments. Pass them as a JSON object string:

```
ubus('some.object', 'method', 'result.key', '{"param": "value"}')
```

---

## Caching

Each unique combination of `(path, method, args)` is cached for **2 seconds**. If multiple widgets call `ubus('system', 'info', ...)` with different `jsonpath` values, only one actual ubus call is made per 2-second window. This is transparent and requires no configuration.

---

## Troubleshooting

**`ubus: cannot connect to ubusd`**  
ubusd is not running, or the socket path is wrong. Check with:
```sh
ubus list
ls -la /var/run/ubus/
```

**`ubus: object 'x' not found`**  
The object does not exist on this system. Explore available objects with:
```sh
ubus list
ubus list -v
```

**Empty result from `jsonpath`**  
The path does not exist in the response. Inspect the full response by calling `ubus()` without a jsonpath argument, or use the CLI:
```sh
ubus call system info
ubus call network.interface.wan status
```
