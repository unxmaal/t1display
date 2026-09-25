# Rule 01: Project Overview

## What This Is

t1display is ESP32-S3 firmware for the M5Stack CoreS3 that displays Nightscout
CGM (continuous glucose monitor) data as a bedside monitor. It fetches blood
glucose readings from a Nightscout server and shows them with trend arrows,
color-coded ranges, and melodic audio alarms.

The name is a play on T1D, type 1 diabetes.

## Lineage

Originally forked from mlukasek/M5_NightscoutMon, then rewritten ground-up for
the CoreS3. No upstream source files remain. Upstream targeted the M5Stack
Core/Core2 with the Arduino IDE and a single ~2900-line .ino.

The GPL-3.0 LICENSE is the one upstream artifact deliberately retained.

## License

GNU General Public License v3.0 or later. Copyright 2024-2026 Eric Dodd.
Every source file under `cores3/` and `lib/` carries an SPDX-License-Identifier
header. Test suites do not.

## Hardware Target

Single target: **M5Stack CoreS3** (ESP32-S3, 320x240 IPS touch display, I2S
speaker). No compile-time board branching. Optional DIN Base with 500mAh battery.

## Key External Services

- **Nightscout** — the only CGM data source: API v1 `entries.json` for the sgv,
  API v2 `properties/delta` for the delta
- **NTP** — `pool.ntp.org`, `time.nist.gov`, `time.google.com`
- **OTA** — ArduinoOTA over the local network, hostname from `device_name`

## Units

Glucose thresholds are always mmol/L internally. `show_mgdl` affects display
formatting only. See `rules/04-configuration.md`.
