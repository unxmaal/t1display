# Rule 03: Project Structure

## Layout

PlatformIO project. Firmware lives under `cores3/`; hardware-free logic lives in
`lib/` and is compiled by both the firmware and the native test build.

```
cores3/
  platformio.ini        — CoreS3 firmware env + native env
  src/
    main.cpp            — setup(), loop(), SD config load, WiFi bring-up
    display.cpp         — screen rendering, touch zones, pages
    nightscout.cpp      — HTTP fetch and response handling
    alerts.cpp          — alarm state machine, tone generation
    ota.cpp             — ArduinoOTA setup
    webconfig.cpp       — embedded web config UI, writes /M5NS.INI back
  include/              — headers for the above
  diagram.json          — Wokwi simulation layout
  wokwi.toml            — Wokwi config
  load_env.py           — PlatformIO pre-build script
lib/
  ns_pure_logic/        — glucose color, alarm levels, formatting, JSON sanitising
  ns_json_parse/        — Nightscout API JSON parsing (ArduinoJson)
  ns_display_model/     — display layout as pure data structs
  ns_config_parse/      — INI parse, validate, serialize
test/native/            — Unity suites, one subdirectory per suite
SD/M5NS.INI             — sample SD card config, covered by test_sample_ini
rules/                  — project rules for Claude sessions
tools/knowledge-server.py — knowledge MCP server
platformio.ini          — root, native test env only
README.md               — the only prose documentation in this repo
```

## Build System

**Firmware:** `cd cores3 && pio run` (env `m5stack-cores3`).
**Tests:** `pio test -e native` from the repository root.

See `rules/09-testing.md` for test architecture and TDD workflow.

## Key Dependencies

| Library | Source | Purpose |
|---------|--------|---------|
| M5Unified | Library manager | CoreS3 hardware abstraction |
| ArduinoJson 7 | Library manager | Nightscout JSON parsing |
| WiFi, WiFiMulti, HTTPClient, WebServer, ESPmDNS, ArduinoOTA | ESP32 core | Networking |

## Adding New Files

**Firmware code** goes in `cores3/src/` with its header in `cores3/include/`.
Every new source file needs the SPDX header block.

**Pure logic** (no Arduino/hardware deps) goes in `lib/<name>/`. This is picked
up by `lib_extra_dirs` for both the firmware and native test builds. Prefer
putting logic here so it can be tested on the host.

**Test suites** go in `test/native/<suite_name>/<suite_name>.cpp`. Each suite
must be in its own subdirectory — PlatformIO will not find flat `.cpp` files.

## Documentation

All prose documentation belongs in `README.md`. Do not add other `.md` files
outside `rules/`, and do not reintroduce a CHANGELOG — git history is the log.
