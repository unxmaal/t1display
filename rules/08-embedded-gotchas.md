# Rule 08: Embedded / ESP32 Gotchas

## Memory

- ESP32-S3 has ~512KB internal SRAM plus 8MB PSRAM. Free internal heap is shown on the system page.
- ArduinoJson 7 `JsonDocument` is a small stack object whose storage is heap-allocated
  (`malloc`) on demand, per parse.
- The 320x240x16bpp canvas is 153,600 bytes and must live in PSRAM (`setPsram(true)`).
- Avoid dynamic allocation in loops. Prefer stack or global buffers.

## Timing

- `loop()` runs continuously. Timing is managed via `millis()` comparisons, not `delay()`.
- Millisecond values in `lib/` are `uint32_t`, never `unsigned long`. `millis()`
  is 32-bit on the ESP32 but `unsigned long` is 64-bit on the native test host,
  so an `unsigned long` rollover test never actually wraps. Compare with
  unsigned subtraction, or `(int32_t)(a - b)` for ordering.
- The web server runs on its own task (`webTask`), polling `handleClient()` every 5 ms.
- Nightscout polling: `loop()` checks every 15 seconds and asks nsTask to fetch once
  the reading is 5 minutes old.
- Don't add blocking operations to `loop()` — they freeze the display, touch and alarms.

## Persistence

No NVS/Preferences use. The SD card `/M5NS.INI` is the only persisted state;
snooze and restart state live in RAM and reset on power cycle.

## I2C Bus

- Managed by M5Unified for the CoreS3 internal peripherals
- No external sensors are wired up in this build

## Power

- `M5.Power.getBatteryLevel()` returns 0-100

## Watchdog / Restart

- The task watchdog is `NS_WDT_TIMEOUT_SEC`. nsTask resets it between the two
  Nightscout fetches, and `NS_FETCH_WINDOW_MS` (one fetch's connect + read
  timeout) must stay under half of it, which `test_runtime` asserts. Raise a
  timeout only with that test in view.

- Scheduled restart via `restart_at_time` config (HH:MM format)
- Error-triggered restart via `restart_at_logged_errors`
- Snooze state is RAM-only and does not survive a restart

## Build Defines

- `ARDUINOJSON_USE_LONG_LONG` is already the default on 32-bit targets in ArduinoJson 7; the explicit define is belt and braces
- `WOKWI_SIM` — set only for simulator builds
