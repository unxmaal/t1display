# Rule 08: Embedded / ESP32 Gotchas

## Memory

- ESP32 has ~520KB SRAM. Free heap is shown on the error log page (page 3).
- The 16KB `DynamicJsonDocument` is a significant allocation — it's a global, reused.
- Audio buffers are large: 25,000 elements (25KB for BASIC, 50KB for Core2 int16_t).
- Avoid dynamic allocation in loops. Prefer stack or global buffers.

## Timing

- `loop()` runs continuously. Timing is managed via `millis()` comparisons, not `delay()`.
- Web server processing happens every 20ms (`msCount` check).
- Nightscout API polling: every 15 seconds, but only fetches when data age > 5 minutes.
- Don't add blocking operations to `loop()` — they freeze the display and web server.

## Persistence

No NVS/Preferences use. The SD card `/M5NS.INI` is the only persisted state;
snooze and restart state live in RAM and reset on power cycle.

## I2C Bus

- Managed by M5Unified for the CoreS3 internal peripherals
- No external sensors are wired up in this build

## Power

- BASIC/GRAY/FIRE: `M5.Power.getBatteryLevel()` returns 0-100
- Core2: `M5.Axp.GetBatVoltage()` mapped to 0/25/50/75/100%
- Power off (BASIC): `M5.Power.setWakeupButton(BUTTON_A_PIN)` + `M5.Power.powerOFF()`

## Watchdog / Restart

- Scheduled restart via `restart_at_time` config (HH:MM format)
- Error-triggered restart via `restart_at_logged_errors`
- Soft restart preserves snooze state through NVS

## Build Defines

- `ARDUINOJSON_USE_LONG_LONG 1` — MUST be defined before `#include <ArduinoJson.h>`
- `ARDUINO_M5STACK_Core2` — defined by the board package, not by us
