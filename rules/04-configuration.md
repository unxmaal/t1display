# Rule 04: Configuration System

## Source of truth

Single source: **SD card INI** at `/M5NS.INI`, read in `cores3/src/main.cpp`
and parsed by `parseConfigBuffer()` in `lib/ns_config_parse/`. The web config UI
(`cores3/src/webconfig.cpp`) writes the same file back via `serializeConfigINI()`.
There is no NVS/Preferences fallback and no bootstrap AP mode.

## Struct

`ParsedConfig` in `lib/ns_config_parse/ns_config_parse.h`. Notable fields:

- `url[128]`, `token[64]`, `userName[32]`, `deviceName[32]`
- `yellow_low/high`, `red_low/high` — display color thresholds
- `snd_alarm`, `snd_warning`, `snd_alarm_high`, `snd_warning_high` — audio thresholds
- `wlanssid[10][64]`, `wlanpass[10][64]`
- `unknownKeys` — count of keys the parser did not recognize
- `configErrors`, `firstBadKey` — everything that was not taken as written

## Units: thresholds are always mmol/L

`show_mgdl` controls **display formatting only**. It does not rescale thresholds.
All threshold fields are compared against mmol/L values in `glucoseColor()` and
`alarmLevel()` regardless of `show_mgdl`.

In the INI, a threshold above `CFG_GLUCOSE_MAX` (40) that fits the mg/dL range
is converted, so upstream mg/dL INIs load correctly. `validateConfig()` enforces
ordering on the colour bands and on the alarm thresholds, reverting a
mis-ordered set to defaults.

## INI format

Sections are `[config]` and `[wlan1]`–`[wlan10]` (`wlan1` maps to index 0),
matched case-insensitively.
Values are unquoted; everything after the first `=` is taken verbatim after
whitespace trimming. An SSID or password containing spaces needs no quotes —
quoting it makes the quotes part of the value. Max 63 characters each.

## Unknown keys

`parseConfigBuffer()` increments `cfg->unknownKeys` for any key it does not
recognize, in both `[config]` and `[wlan*]` sections. A correct INI parses with
`unknownKeys == 0`. This is asserted against the shipped `SD/M5NS.INI` by
`test/native/test_sample_ini/`.

## Config errors

Every rejection, clamp, truncation, unknown section or key, bad
`restart_at_time` and threshold-order violation calls `configError()`.
`formatConfigErrors()` renders the summary shown on the splash, the status page
and the web save page. `applyConfigForm()` resets the count first. Never add a
path that silently discards user input.

## Adding a new config field

1. Add the field to `ParsedConfig` in `ns_config_parse.h`
2. Set its default in `configDefaults()`
3. Add a `strcmp(key, ...)` branch in `parseConfigBuffer()` before the final `else`
4. Emit it from `serializeConfigINI()`
5. Clamp it in `validateConfig()` if it has a valid range
6. Add the form field and `/savecfg` handling in `cores3/src/webconfig.cpp`
7. Add it to `SD/M5NS.INI`, or `test_sample_ini` roundtrip coverage will not see it
8. Add native tests in `test/native/test_config_parse/`
