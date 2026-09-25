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
source files remain.

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
tz = EST5EDT,M3.2.0,M11.1.0
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

### Staleness

The display has three states, because a stale reading that still looks current is
the most dangerous thing this device can show.

| State | Age | Appearance |
|-------|-----|------------|
| Fresh | up to 10 min | value in range colour |
| Stale | 11–19 min | value greyed and struck through, age shown |
| No data | 20 min or more, or age unknown | value replaced by `--.-`, `NO DATA` banner, last known value labelled below |

Fresh runs to 10 minutes because readings arrive every 5 minutes and uploaders
add a few minutes of lag; a reading only goes stale once one has been missed.

A reading whose age cannot be determined — no NTP sync, no timestamp from the
server, or a timestamp more than 2 minutes in the future — is treated as no data
rather than assumed fresh. A fast phone clock on the uploader would otherwise keep
a dead feed looking live.

An sgv below 39 mg/dL is a CGM error code (sensor warm-up, `???`, signal loss),
not a reading. It shows `--.-` with a `SENSOR ERROR` banner and raises the
no-readings alert instead of a low alarm.

mg/dL and mmol/L convert at 18.01559, the factor Nightscout uses.

### Clock and dates

`tz` takes a POSIX TZ string, which carries the daylight-saving rules, so the
clock changes on its own twice a year. Examples:
- `UTC0`
- `GMT0BST,M3.5.0/1,M10.5.0` (UK)
- `CET-1CEST,M3.5.0,M10.5.0/3` (central Europe)
- `EST5EDT,M3.2.0,M11.1.0` (US Eastern)
- `AEST-10AEDT,M10.1.0,M4.1.0/3` (Sydney)

POSIX puts the sign the opposite way from UTC offsets: `EST5` means five hours
behind UTC. An invalid `tz` counts as a config error. The older `time_zone` and
`dst` keys, fixed offsets in seconds, are used only when `tz` is empty. With
neither set, the device runs on UTC.

`time_format = 0` shows 24-hour time, `1` shows 12-hour (`1:07p`).
`date_format = 0` writes error-log dates day first (`03.04`), `1` month first
(`04/03`). An error logged before the clock synced shows `no clock`.
`default_page` picks the page shown at boot: 0 glucose, 1 error log, 2 system.

### Trend arrows

One arrow for the usual Nightscout directions. `TripleUp` and `TripleDown`, the
fastest rise and fall, draw two arrows. `RATE OUT OF RANGE` draws an up-and-down
pair. `NONE`, `NOT COMPUTABLE` and anything unrecognised draw nothing. Numeric
`trend` values 0–9 map onto the same names.

### Thresholds are always mmol/L

`show_mgdl` controls **display formatting only**. Every threshold — `yellow_*`,
`red_*`, and all four `snd_*` values — is compared in mmol/L no matter how
`show_mgdl` is set.

A threshold above 40 can only be mg/dL, so in the INI it is converted rather than
rejected. That makes an upstream M5_NightscoutMon INI, where `show_mgdl = 1`
meant the values were mg/dL, load correctly. Values of 40 and below are always
read as mmol/L.

The thresholds must be in order: `red_low ≤ yellow_low < yellow_high ≤ red_high`
and `snd_alarm ≤ snd_warning < snd_warning_high ≤ snd_alarm_high`. If either set
is out of order, that whole set reverts to its defaults. A mis-ordered set could
otherwise turn a hyper emergency into a low-warning chime.

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

### Security

Three optional keys, all empty by default:

```ini
ota_password = your-ota-secret
web_user = yourname
web_pass = your-web-secret
```

`ota_password` gates over-the-air updates. **With it unset, OTA does not start at
all** — the port stays closed rather than accepting unauthenticated firmware.
Set it before relying on OTA; the `ota` build env passes it as `--auth`.

`web_user` and `web_pass` together enable HTTP Basic auth on the config UI. With
either unset there is no authentication, and anyone on your network can change
the config, reboot the device or play alarm sounds. Basic auth over plain HTTP
is weak, but it is the difference between needing a credential and needing
nothing.

The page never shows a stored secret: the Nightscout token and Wi-Fi passwords
appear as empty password fields marked "set" or "not set". Leave one blank to
keep it; tick its "clear" box to remove it.

### Sections

`[config]` for everything else, `[wlan1]` through `[wlan10]` for networks.
Section names ignore case.

### Config errors

Anything the device could not take as written counts as a config error:
- an unknown section or key;
- a value that doesn't parse, is out of range (clamped), or is too long (truncated);
- a line over 255 characters;
- a `restart_at_time` that isn't `HH:MM` (reverts to `NORES`);
- a threshold set out of order (reverts to defaults).

The count and the first offending key appear in yellow on the boot splash, on the
system page, over serial, and on the page shown after a web save, e.g.
`2 config errors: red_hihg`. A web save clears errors from the file it replaced.

## Touch controls

The CoreS3 touch zones sit along the bottom of the screen. When no alarm is
showing they are labelled `BRIGHT` and `PAGE`. During an alarm the bar across
the bottom reads `TAP TO SNOOZE`, then `SNOOZED n min`. The bar stops short of
the error badge and the battery icon, so both stay visible.

| Zone | Action |
|------|--------|
| Left third | Cycle brightness |
| Centre third | Snooze alarms |
| Right third | Switch page: glucose → error log → system |

Snooze lasts `snooze_timeout` minutes (at least 1) and covers only the alarm you
snoozed and milder ones in the same direction. Snoozing a low never silences a
high, and neither silences a no-readings alert. A second tap within 10 seconds
counts as one; a later tap adds another `snooze_timeout`, up to an hour.

## Building

Requires [PlatformIO](https://platformio.org/).

```bash
# Build firmware
cd cores3 && pio run

# Flash over USB (first time); the port is auto-detected
cd cores3 && pio run -t upload

# Flash over USB to a specific port
cd cores3 && pio run -t upload --upload-port /dev/ttyACM0

# Flash over Wi-Fi once ota_password is set
cd cores3 && T1DISPLAY_HOST=t1display.local T1DISPLAY_OTA_PASSWORD=your-ota-secret \
  pio run -e ota -t upload

# Run the native test suite
pio test -e native

# Serial monitor
cd cores3 && pio device monitor -b 115200

# Build for the Wokwi simulator (joins the Wokwi-GUEST virtual network)
cd cores3 && pio run -e wokwi
```

The `Wokwi-GUEST` open network is only joined in simulator builds. Release
firmware never associates with it.

CI runs the `wokwi` build in the Wokwi simulator as a boot smoke test. It
passes once serial shows `[BOOT] Setup complete` without a `Guru Meditation`
panic within 90 seconds. It needs a `WOKWI_CLI_TOKEN` repository secret (free
plan: 50 CI minutes a month); without one the job builds and then skips the
run with a notice. The job is non-blocking, and its serial log is uploaded as
an artifact.

## Architecture

Hardware-dependent code lives in `cores3/`. Everything that can be tested on the
host lives in `lib/` and is compiled into both the firmware and the native tests.

- `cores3/src/` — CoreS3 firmware: display, WiFi, alerts, OTA, web config
  - Nightscout fetches and the web server each run on their own FreeRTOS
    task on core 0, so a slow network request or browser cannot stall touch
    input, the snooze button, or alarm checks on the core-1 loop.
  - The loop owns the config, the SD card, the display and the speaker. Other
    tasks read the config through a locked snapshot, submit saves and test
    sounds as requests, and never touch the SPI bus or the speaker.
- `lib/ns_pure_logic/` — glucose colour, alarm levels, formatting, JSON sanitising
- `lib/ns_json_parse/` — Nightscout API JSON parsing
- `lib/ns_display_model/` — display layout as pure data structs
- `lib/ns_config_parse/` — INI parsing, validation, and serialization
- `lib/ns_runtime/` — watchdog and HTTP timeout budget, network service start latch
- `lib/ns_shared/` — `Guarded<T>` and `Exchange<Req, Resp>`, the locked handoffs between tasks
- `test/native/` — Unity test suites, one directory per suite
- `rules/` — project rules consumed by the knowledge MCP server
- `tools/` — the knowledge MCP server itself

## Testing

15 native suites, 202 assertions, no hardware required:

```bash
pio test -e native
pio test -e native_asan                      # address and undefined behaviour
TSAN_OPTIONS=halt_on_error=1 pio test -e native_tsan   # data races
pio check -e native --fail-on-defect=medium   # cppcheck
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
