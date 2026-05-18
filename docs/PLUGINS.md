# Plugin and Expression Function Reference

All plugins are loaded automatically at startup. No configuration block is needed to use them (except `ubus`, see [UBUS.md](UBUS.md)).

Expression functions are used in widget `value`, `text`, `visible`, and similar keys.

---

## CPU — `cpuinfo`

```
cpu::load()           # total CPU usage, 0–100 (float)
cpu::info()           # CPU model name string
```

`cpu::load()` samples CPU usage over the widget's update interval.

---

## Memory — `meminfo`

```
mem::total('kb'|'mb'|'gb'|'%')    # total RAM
mem::used('kb'|'mb'|'gb'|'%')     # used RAM
mem::free('kb'|'mb'|'gb'|'%')     # free RAM

mem::swap::total('kb'|'mb'|'gb'|'%')
mem::swap::used('kb'|'mb'|'gb'|'%')
mem::swap::free('kb'|'mb'|'gb'|'%')
```

The argument selects the unit. `'%'` or strings starting with `p` (e.g. `'percent'`) return a 0–100 percentage.

**Examples:**
```
text  'RAM: ' . format(mem::used('mb'), 0) . ' / ' . format(mem::total('mb'), 0) . ' MB'
value mem::used('percent')
```

---

## Uptime — `uptime`

```
uptime()              # "Xd Xh Xm" — days, hours, minutes (no seconds)
uptime::long()        # "Xd Xh Xm Xs" — includes seconds
uptime::days()        # days component (number)
uptime::hours()       # hours component (number)
uptime::minutes()     # minutes component (number)
uptime::seconds()     # seconds component (number)
uptime::timestamp()   # total uptime in seconds (number)
```

`uptime()` and `uptime::long()` return human-readable strings. For accurate second-level display, set `update 1000` on the widget.

---

## Filesystem — `fs`

```
fs::size('/path'[, 'kb'|'mb'|'gb'])      # total filesystem size
fs::capacity('/path'[, 'kb'|'mb'|'gb'])  # same as fs::size
fs::used('/path'[, 'kb'|'mb'|'gb'])      # used space
fs::free('/path'[, 'kb'|'mb'|'gb'])      # free space
fs::available('/path'[, 'kb'|'mb'|'gb']) # available space (excludes reserved)

fs::size::hr('/path')        # human-readable string (e.g. "14.2 GB")
fs::capacity::hr('/path')
fs::used::hr('/path')
fs::free::hr('/path')
fs::available::hr('/path')
```

Default unit is MB when the second argument is omitted.

**Example:**
```
text  'Root: ' . fs::used::hr('/') . ' / ' . fs::size::hr('/')
```

---

## Network — `netinfo`

```
netinfo::exists('iface')          # 1 if interface exists
netinfo::encap('iface')           # encapsulation type string
netinfo::operstate('iface')       # operational state ('up', 'down', ...)
netinfo::mtu('iface')             # MTU
netinfo::hwaddr('iface')          # MAC address string
netinfo::ip4addr('iface')         # IPv4 address string
netinfo::netmask('iface')         # IPv4 netmask string
netinfo::cidrmask('iface')        # CIDR prefix length (number)
netinfo::bcaddr('iface')          # broadcast address string
netinfo::ip6addr('iface')         # IPv6 address string
netinfo::prefix('iface')          # IPv6 prefix length (number)
netinfo::scope('iface')           # IPv6 scope string
netinfo::rx::packets('iface')     # received packets
netinfo::rx::bytes('iface')       # received bytes
netinfo::tx::packets('iface')     # transmitted packets
netinfo::tx::bytes('iface')       # transmitted bytes
```

---

## System info — `uname`

```
uname::sysname()      # OS name, e.g. "Linux"
uname::nodename()     # hostname
uname::hostname()     # alias for nodename
uname::release()      # kernel release, e.g. "6.1.0"
uname::kernel()       # alias for release
uname::version()      # kernel build version string
uname::build()        # alias for version
uname::machine()      # hardware architecture, e.g. "x86_64", "aarch64"
uname::domainname()   # NIS domain name
```

---

## File — `file`

```
file::readline('/path/to/file'[, linenum])  # read a line from a file
file::readconf('/path/to/file', 'key')      # read key=value from a config file
file::exists('/path/to/file')               # 1 if file exists, 0 otherwise
```

`file::readline` reads line number `linenum` (1-based; default 1).
`file::readconf` reads the value of a key from a simple `key=value` or `key: value` file.

**Examples:**
```
text  file::readline('/sys/class/thermal/thermal_zone0/temp')
text  file::readconf('/etc/hostname', 'hostname')
```

---

## Exec — `exec`

Runs an external command and returns its stdout as a string.

```
exec('command [args...]')
exec('/path/to/script.sh arg1 arg2')
```

The command is passed through a shell-like parser that handles quoted arguments. Output is trimmed of trailing whitespace.

**Examples:**
```
text  exec('date +%Y-%m-%d')
value exec('/usr/local/bin/sensor-read.sh')
```

---

## Test — `test`

```
test::on_off()    # alternates between 0 and 1 on each call
```

Useful for testing blinking or toggling widgets.

---

## Built-in expression functions

These functions are always available, independent of plugins.

| Function | Description |
|---|---|
| `strftime(format, time())` | Format a Unix timestamp as a date/time string |
| `time()` | Current Unix timestamp |
| `format(value, decimals)` | Format a number to N decimal places |
| `round(value)` | Round to nearest integer |
| `floor(value)` | Round down |
| `ceil(value)` | Round up |
| `abs(value)` | Absolute value |
| `min(a, b)` | Minimum of two values |
| `max(a, b)` | Maximum of two values |
| `clamp(value, min, max)` | Clamp value to [min, max] range |

---

## ubus (optional)

Available only when built with `WITH_UBUS=1`. See [UBUS.md](UBUS.md) for full documentation.

```
ubus('path', 'method')                     # full JSON response as string
ubus('path', 'method', 'json.path')        # value at dot-notation path
ubus('path', 'method', 'json.path', args)  # with method arguments
```
