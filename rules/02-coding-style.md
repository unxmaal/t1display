# Rule 02: Coding Style

## Indentation

4 spaces. No tabs.

## Naming Conventions

This is a mixed-convention codebase. **Match the surrounding code**, don't try to unify.

| Context | Convention | Examples |
|---------|-----------|----------|
| Functions | camelCase | `readNightscout()`, `drawGlucosePage()`, `sensorAgeMinutes()` |
| Global variables | camelCase | `currentPage`, `brightnessLevel`, `lastNsCheck`, `nsFetchRequested` |
| Struct fields | match the struct | `ParsedConfig` INI-backed fields follow their key (`cfg.show_mgdl`, `cfg.snd_warning`); `NSinfo` is camelCase (`ns.sensSgv`, `ns.arrowAngle`) |
| Local variables | camelCase | `httpCode`, `sensorAgeMin`, `nowMs` |
| Constants/defines | UPPER_SNAKE | `NUM_PAGES`, `CFG_MAX_WLAN`, `SENSOR_AGE_STALE_MIN` |

## Include Patterns

- System/library headers: angle brackets — `<Arduino.h>`, `<WiFi.h>`, `<ArduinoJson.h>`
- Project-local files: double quotes — `"ns_config_parse.h"`, `"display.h"`

## String Handling

C-style buffers in `lib/`, which must stay free of Arduino types. Arduino `String` is allowed in `cores3/src/` only:
- C-style `char[]` with `sprintf`/`strlcpy`/`strncpy` for config fields and fixed buffers
- Arduino `String` for dynamic content (web server HTML, version strings)

## Hardware Conditionals

One board target. The build conditionals are `WOKWI_SIM` (the `wokwi` env) and
`TEST_NS_URL` / `TEST_NS_TOKEN`, which `load_env.py` injects from an untracked
`cores3/.env` as a no-SD fallback for bench testing. Never ship a build with
`.env` present.

## Don't "Improve" Existing Style

This is a community-maintained embedded project. Don't refactor naming, add type annotations, modernize C++ idioms, or "clean up" code you weren't asked to change.
