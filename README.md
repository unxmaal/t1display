# t1display

A bedside CGM display for the M5Stack CoreS3 — Type 1, at a glance.

Shows real-time glucose data from [Nightscout](https://nightscout.github.io/)
with colour-coded ranges, trend arrows, and melodic audio alerts. The name is a
play on T1D.

Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
Licensed under the GNU General Public License v3.0 or later. See [LICENSE](LICENSE).

## Originally based on

[M5_NightscoutMon](https://github.com/mlukasek/M5_NightscoutMon) by Martin
Lukasek (Copyright 2018-2020), with contributions from Peter Leimbach, Patrick
Sonnerat, Sulka Haro, and Dominik Dzienia. That project targeted the M5Stack
Core/Core2 using the Arduino IDE.

This is a ground-up rewrite for the M5Stack CoreS3 with a new architecture,
pure-logic libraries, a native test suite, and a PlatformIO build. No upstream
source files remain. A handful of upstream string literals survive where they
are protocol rather than prose — most importantly the UDP snooze packet format,
which is kept byte-identical so snooze still syncs with upstream devices on the
same network.

## Features

- Large, dark-room-friendly glucose display, colour-coded by range
- Melodic audio alerts with distinct tones for low/high warnings and alarms
- Touch controls for brightness, snooze, and page switching
- Web configuration UI, reachable from any browser on your network
- OTA firmware updates over WiFi
- Multi-WiFi support, up to 10 networks
- SD card configuration via `M5NS.INI`

## Hardware

- **M5Stack CoreS3** (ESP32-S3, 320x240 IPS touch display, I2S speaker)
- **MicroSD card** (FAT32, MBR partition scheme) for configuration
- Optional: DIN Base with 500mAh battery. It has a physical power switch that
  must be ON for battery operation.

## Quick start

1. Format a microSD card as FAT32 (MBR, not GPT)
2. Copy [`SD/M5NS.INI`](SD/M5NS.INI) to the card root and edit it
3. Insert the card into the CoreS3
4. Flash the firmware:
   ```bash
   cd cores3 && pio run -t upload
   ```
5. Once it joins WiFi, open `http://t1display.local` for the web config

## Configuration

The device reads `/M5NS.INI` from the SD card root at boot. The web UI writes the
same file back, so the two never diverge.

```ini
[config]
nightscout = https://your-nightscout-site.example.com
token = your-api-token
name = YourName
device_name = t1display
time_zone = 3600
show_mgdl = 1
yellow_low = 4.5
yellow_high = 9.0
red_low = 3.9
red_high = 11.0
snd_warning = 3.7
snd_alarm = 3.0
snd_warning_high = 14.0
snd_alarm_high = 20.0

[wlan1]
ssid = Your Network Name
pass = YourWiFiPassword
```

### Thresholds are always mmol/L

`show_mgdl` controls **display formatting only**. Every threshold — `yellow_*`,
`red_*`, and all four `snd_*` values — is compared in mmol/L no matter how
`show_mgdl` is set.

This differs from upstream M5_NightscoutMon, where `show_mgdl = 1` meant the INI
values themselves were in mg/dL. Do not copy threshold numbers from an upstream
INI: parsed as mmol/L, a value like `yellow_low = 80` makes every normal reading
register as out of range, and `snd_alarm = 60` alarms on every reading. The
shipped sample file is checked against this by `test/native/test_sample_ini/`.

### Values are not quoted

Everything after the first `=` is taken verbatim once surrounding whitespace is
trimmed. An SSID or password containing spaces needs **no quotes** — adding them
makes the quote characters part of the value, and the network will not be found.

```ini
ssid = My Network Name      ✓
ssid = "My Network Name"    ✗ connects to  "My Network Name"  including quotes
```

Leading and trailing spaces cannot be represented, since they are trimmed. SSID
and password are limited to 63 characters each. `#` and `;` only start a comment
at the beginning of a line.

### Sections

`[config]` for everything else, `[wlan1]` through `[wlan10]` for networks. Any
key the parser does not recognise is counted in `unknownKeys` and otherwise
ignored, so a stale INI fails loudly in tests rather than silently at runtime.

## Touch controls

The CoreS3 touch zones sit along the bottom of the screen.

| Zone | Action |
|------|--------|
| Left third | Cycle brightness |
| Centre third | Snooze alarms |
| Right third | Switch page |

## Building

Requires [PlatformIO](https://platformio.org/).

```bash
# Build firmware
cd cores3 && pio run

# Flash over USB (first time)
cd cores3 && pio run -t upload

# Run the native test suite
pio test -e native

# Serial monitor
cd cores3 && pio device monitor -b 115200
```

After the first USB flash, OTA updates work by setting `upload_protocol = espota`
and `upload_port = t1display.local` in `cores3/platformio.ini`.

Note that `cores3/platformio.ini` pins `upload_port` to a specific USB device
path. Adjust it for your machine — typically `/dev/ttyACM0` on Linux.

## Architecture

Hardware-dependent code lives in `cores3/`. Everything that can be tested on the
host lives in `lib/` and is compiled into both the firmware and the native tests.

- `cores3/src/` — CoreS3 firmware: display, WiFi, alerts, OTA, web config
- `lib/ns_pure_logic/` — glucose colour, alarm levels, formatting, snooze, CRC
- `lib/ns_json_parse/` — Nightscout API JSON parsing
- `lib/ns_display_model/` — display layout as pure data structs
- `lib/ns_config_parse/` — INI parsing, validation, and serialization
- `test/native/` — Unity test suites, one directory per suite
- `rules/` — project rules consumed by the knowledge MCP server
- `tools/` — the knowledge MCP server itself

## Testing

15 native suites, 202 assertions, no hardware required:

```bash
pio test -e native
```

New logic belongs in `lib/` with a matching suite in
`test/native/<name>/<name>.cpp`. Each suite needs its own subdirectory —
PlatformIO will not discover flat `.cpp` files.

`test_sample_ini` is unusual in that it parses the actual shipped
`SD/M5NS.INI` rather than a fixture, so the file users are told to copy cannot
drift away from what the firmware expects.

## History

Forked from M5_NightscoutMon in 2023 and rewritten for the CoreS3 across 2025-2026:
new architecture, pure-logic libraries with native tests, PlatformIO build, web
config UI, OTA updates, and melodic alerts. Upstream's own changelog, covering
2018 through October 2022, is preserved in the git history rather than repeated
here.

## License

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

See [LICENSE](LICENSE) for the full text.
