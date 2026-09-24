# Rule 02: Coding Style

## Indentation

2 spaces. No tabs.

## Naming Conventions

This is a mixed-convention codebase. **Match the surrounding code**, don't try to unify.

| Context | Convention | Examples |
|---------|-----------|----------|
| Functions | camelCase | `readNightscout()`, `drawGlucosePage()`, `sensorAgeMinutes()` |
| Global variables | camelCase | `dispPage`, `lcdBrightness`, `snoozeUntil`, `lastAlarmTime` |
| Struct fields | snake_case | `cfg.show_mgdl`, `cfg.snd_warning`, `ns.sensSgv`, `ns.sensDir` |
| Local variables | camelCase | `httpCode`, `sensorDifSec`, `glColor` |
| Constants/defines | UPPER_SNAKE | `NUM_PAGES`, `CFG_MAX_WLAN`, `SENSOR_AGE_STALE_MIN` |

**Exception:** Some functions use snake_case (`wifi_connect`, `draw_page`). Don't rename them.

## Include Patterns

- System/library headers: angle brackets — `<Arduino.h>`, `<WiFi.h>`, `<ArduinoJson.h>`
- Project-local files: double quotes — `"ns_config_parse.h"`, `"display.h"`

## String Handling

C-style buffers in `lib/`, which must stay free of Arduino types. Arduino `String` is allowed in `cores3/src/` only:
- C-style `char[]` with `sprintf`/`strlcpy`/`strncpy` for config fields and fixed buffers
- Arduino `String` for dynamic content (web server HTML, version strings)

## Hardware Conditionals

One board target. The only build conditional is `WOKWI_SIM` for simulator-only
behaviour.

## Don't "Improve" Existing Style

This is a community-maintained embedded project. Don't refactor naming, add type annotations, modernize C++ idioms, or "clean up" code you weren't asked to change.
