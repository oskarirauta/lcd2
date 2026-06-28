# Plugin and Expression Function Reference

All plugins are loaded automatically at startup. No configuration block is needed to use them (except `ubus`, see [UBUS.md](UBUS.md)).

Expression functions are used in widget `value`, `text`, `visible`, and similar keys. Each plugin registers its functions via `CONFIG::functions.append({ "name", fn })` in its constructor; the exact names below are taken verbatim from the source.

Return types below reflect what the function actually returns: a **number** (double), a **string**, or a **bool**.

---

## CPU — `cpuinfo`

| Function | Args | Returns | Description |
|---|---|---|---|
| `cpu::info` | `key` [, `cpu_index`] | string | Returns a `/proc/cpuinfo`-style field. `key` is **required** and must be one of: `vendor`, `family`, `model`, `mhz`, `cache`, `cores`, `stepping`, `microcode`, `fpu`, `bogomips`, `cache_alignment`. Optional second arg is a 0-based CPU index; omitted = whole-CPU/aggregated value. Returns `""` on an invalid/empty key or out-of-range index. |
| `cpu::load` | [`cpu_index`] | number | Total CPU usage. Optional 0-based core index returns that core's load; an out-of-range or non-numeric arg falls back to total load. |

The `cpuinfo` plugin refreshes its samples on a fixed internal interval of **850 ms** (not the widget's update interval). For smooth load display, a widget update around 1000 ms is reasonable.

> **Platform note:** `cpu::info` reads `/proc/cpuinfo`, whose fields differ by
> architecture. x86/x86-64 exposes `model` (model name), `mhz`, `cores`, `vendor`,
> etc.; ARM/aarch64 exposes none of those — only fields like `bogomips`. Requesting
> a key that the running CPU does not provide logs an error and returns `""`. Pick
> keys appropriate to your target (or rely on `cpu::load()`, which is universal).

**Example:**
```
text  'CPU: ' . format(cpu::load(), 0) . '%  (' . cpu::info('model') . ')'   # x86
text  'CPU: ' . format(cpu::load(), 0) . '%'                                 # portable
```

---

## Memory — `meminfo`

| Function | Args | Returns | Description |
|---|---|---|---|
| `mem::total` | [`unit`] | number | Total RAM. |
| `mem::used` | [`unit`] | number | Used RAM. |
| `mem::free` | [`unit`] | number | Free RAM. |
| `mem::swap::total` | [`unit`] | number | Total swap. |
| `mem::swap::used` | [`unit`] | number | Used swap. |
| `mem::swap::free` | [`unit`] | number | Free swap. |

`unit` is one of `kb`, `mb`, `gb`, `%` (or any string starting with `p`, e.g. `percent`). The default when omitted is **`mb`**. Percentage results are clamped to 0–100.

**Examples:**
```
text  'RAM: ' . format(mem::used('mb'), 0) . ' / ' . format(mem::total('mb'), 0) . ' MB'
value mem::used('percent')
```

---

## Network — `netinfo`

All functions take the interface name as their first argument.

| Function | Args | Returns | Description |
|---|---|---|---|
| `netinfo::exists` | `iface` | bool | True if the interface exists. |
| `netinfo::encap` | `iface` | string | Encapsulation type. |
| `netinfo::operstate` | `iface` | string | Operational state (`up`, `down`, ...). |
| `netinfo::mtu` | `iface` | number | MTU (0 if interface missing). |
| `netinfo::hwaddr` | `iface` | string | MAC address. |
| `netinfo::ip4addr` | `iface` | string | First IPv4 address (`""` if none). |
| `netinfo::netmask` | `iface` | string | First IPv4 netmask. |
| `netinfo::cidrmask` | `iface` | string | First IPv4 CIDR prefix field (string; `""` if none). |
| `netinfo::bcaddr` | `iface` | string | First IPv4 broadcast address. |
| `netinfo::ip6addr` | `iface` | string | First IPv6 address. |
| `netinfo::prefix` | `iface` | string | First IPv6 prefix field (string; `""` if none). |
| `netinfo::scope` | `iface` | string | First IPv6 scope. |
| `netinfo::rx::packets` | `iface` | string | Received packet count. |
| `netinfo::rx::bytes` | `iface` [, `format`] | string | Received byte counter, formatted (see below). |
| `netinfo::tx::packets` | `iface` | string | Transmitted packet count. |
| `netinfo::tx::bytes` | `iface` [, `format`] | string | Transmitted byte counter, formatted (see below). |

`rx::bytes` / `tx::bytes` accept an optional `format`: `auto` (default), `b`/`bytes`, `kb`/`kib`, `mb`/`mib`, `gb`/`gib`. With `auto` the result is a human-readable string with a binary-unit suffix (e.g. `1.50 GiB`, or `N bytes`). Numeric units (`kib`, `mib`, ...) return a bare number string. Units are **binary** (1 KiB = 1024 bytes); `kb`/`mb`/`gb` are treated as aliases of `kib`/`mib`/`gib`.

**Example:**
```
text  'eth0 rx: ' . netinfo::rx::bytes('eth0', 'mib') . ' MiB'
```

---

## Filesystem — `fs`

| Function | Args | Returns | Description |
|---|---|---|---|
| `fs::size` | `path` | number (bytes) | Filesystem capacity for a directory, or file size for a regular file. |
| `fs::capacity` | `path` | number (bytes) | Alias of `fs::size`. |
| `fs::used` | `path` | number (bytes) | Used space (capacity − free); `path` must be a directory. |
| `fs::free` | `path` | number (bytes) | Free space; `path` must be a directory. |
| `fs::available` | `path` | number (bytes) | Available space excluding reserved blocks; `path` must be a directory. |
| `fs::size::hr` | `path` | string | Human-readable form of `fs::size`. |
| `fs::capacity::hr` | `path` | string | Alias of `fs::size::hr`. |
| `fs::used::hr` | `path` | string | Human-readable used space. |
| `fs::free::hr` | `path` | string | Human-readable free space. |
| `fs::available::hr` | `path` | string | Human-readable available space. |

The plain functions return **raw bytes** as a number; there is no unit argument. Use the `::hr` variants for a formatted string (e.g. `14.2 GB`). Errors (missing path, wrong type, stat failure) return `0` / an empty string.

**Example:**
```
text  'Root: ' . fs::used::hr('/') . ' / ' . fs::size::hr('/')
```

---

## System info — `uname`

None of these accept arguments (passing any logs a warning).

| Function | Returns | Description |
|---|---|---|
| `uname::sysname` | string | OS name, e.g. `Linux`. |
| `uname::nodename` | string | Hostname. |
| `uname::hostname` | string | Alias of `uname::nodename`. |
| `uname::release` | string | Kernel release, e.g. `6.1.0`. |
| `uname::kernel` | string | Alias of `uname::release`. |
| `uname::version` | string | Kernel build version string. |
| `uname::build` | string | Alias of `uname::version`. |
| `uname::machine` | string | Hardware architecture, e.g. `x86_64`, `aarch64`. |
| `uname::domainname` | string | NIS/YP domain name. Only functional when built with `_GNU_SOURCE`; otherwise logs an error and returns `""`. |

---

## Uptime — `uptime`

None of these accept arguments.

| Function | Returns | Description |
|---|---|---|
| `uptime` | string | `"Xd Xh Xm"` — days/hours shown only when greater than 0; no seconds. |
| `uptime::long` | string | `"Xd Xh Xm Xs"` — includes seconds. |
| `uptime::days` | number | Days component. |
| `uptime::hours` | number | Hours component. |
| `uptime::minutes` | number | Minutes component. |
| `uptime::seconds` | number | Seconds component. |
| `uptime::timestamp` | number | Total uptime in seconds. |

For accurate second-level display, set a low widget `update` (e.g. `update 1000`).

---

## File — `file`

| Function | Args | Returns | Description |
|---|---|---|---|
| `file::readline` | `path` [, `linenum`] | string | Reads line `linenum` (1-based; default 1). Returns `""` if the file is missing/unreadable or has too few lines. |
| `file::readconf` | `path`, `key` [, `fallback`] | string | Reads a value from a simple config file. The first line whose start matches `key` (case-insensitive prefix match) is used; leading whitespace, `:` and `=` separators are trimmed. Optional `fallback` is returned when the file is missing/unreadable or the value is empty (default `""`). |
| `file::exists` | `path` | bool | True if the path exists. |

**Examples:**
```
text  file::readline('/sys/class/thermal/thermal_zone0/temp')
text  file::readconf('/etc/os-release', 'PRETTY_NAME', 'unknown')
```

---

## Exec — `exec`

| Function | Args | Returns | Description |
|---|---|---|---|
| `exec` | `command line` | string | Runs an external command and returns its stdout. |

The single string argument is parsed with shell-like quote handling (single and double quotes, `\` escaping) into a command plus arguments, then executed via the internal process runner. Unbalanced quotes are auto-closed with a warning.

**Examples:**
```
text  exec('date +%Y-%m-%d')
value exec('/usr/local/bin/sensor-read.sh arg1 "arg with spaces"')
```

---

## Test — `test`

| Function | Args | Returns | Description |
|---|---|---|---|
| `test::on_off` | [`value`] | bool | Returns the logical NOT of its argument. With no argument it assumes `true` and returns `false` (0). It is stateless — it does **not** toggle across calls. |

---

## Built-in expression functions

These are part of the expression engine (`expr/src/function.cpp`,
`builtin_functions` map) and are always available, independent of plugins. Names
joined with `/` below are aliases for the same function.

### Time and date

| Function | Description |
|---|---|
| `time()` / `time::timestamp()` / `time::unixtime()` | Current Unix timestamp |
| `time::hour()` / `time::min()` / `time::sec()` | Current hour / minute / second |
| `date::day()` / `date::month()` / `date::year()` | Current day / month / year |
| `date::weekday()` | Day of week (number) |
| `date::day::name()` | Day-of-week name |
| `strftime(fmt[, time])` / `put_time(fmt[, time])` | Format a timestamp (defaults to now) |

### Numbers and math

| Function | Description |
|---|---|
| `format(value[, decimals])` / `number_format(...)` | Format a number to N decimals (default 2, clamped 0–15) |
| `round` / `floor` / `ceil` / `trunc` / `frac` | Rounding and fractional part |
| `abs` / `fabs` | Absolute value |
| `sign` / `sgn` | Sign (−1/0/1) |
| `min(a,b)` / `max(a,b)` | Minimum / maximum |
| `clamp(v,min,max)` | Clamp to range |
| `map_range(v,inMin,inMax,outMin,outMax)` / `map(...)` | Re-scale between ranges |
| `sqrt` / `exp` / `ln` / `log` | Roots, exponential, logarithms |
| `sin` `cos` `tan` `asin` `acos` `atan` `atan2` `hypot` | Trigonometry |
| `hex` / `to_hex`, `oct` / `to_oct`, `bin` / `to_bin` | Integer base conversion |
| `is_odd` / `is_even` | Parity tests |

### Type conversion

| Function | Description |
|---|---|
| `to_string` | Convert to string |
| `to_int` | Convert to integer |
| `to_double` / `to_number` | Convert to floating point |
| `to_bool` | Convert to boolean |

### Strings

| Function | Description |
|---|---|
| `strlen` / `length` | String length |
| `to_upper` / `strupper`, `to_lower` / `strlower` | Case conversion |
| `substr(s,start[,len])` | Substring |
| `trim` / `strip`, `ltrim`, `rtrim` | Whitespace trimming |
| `pad_left` / `rpad`, `pad_right` / `lpad` | Padding |
| `str_repeat` / `repeat` | Repeat a string |
| `str_contains` / `contains` | Substring test |
| `str_replace` / `replace` | Replace substrings |
| `str_find` / `strpos` | Find substring position |

### Conditional

| Function | Description |
|---|---|
| `if(cond, a, b)` / `iif(...)` / `ternary(...)` | Return `a` if `cond` is non-zero, else `b` |

Expressions also support the inline ternary operator `cond ? a : b`, string
concatenation with `.`, and arithmetic/comparison operators.

---

## ubus (optional)

Available only when built with `WITH_UBUS=1`. See [UBUS.md](UBUS.md) for full documentation.

```
ubus('path', 'method')                     # full JSON response as string
ubus('path', 'method', 'json.path')        # value at dot-notation path
ubus('path', 'method', 'json.path', args)  # with method arguments
```
