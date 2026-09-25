# Rule 05: Display and UI

## Hardware

M5Stack CoreS3: 320x240 IPS, capacitive touch, I2S speaker. One target, no
board conditionals.

## Rendering

Everything is drawn into a full-screen `M5Canvas` and pushed once inside a
`startWrite()`/`endWrite()` pair. The canvas is allocated from PSRAM via
`setPsram(true)` — without it, 153,604 bytes come out of internal DMA-capable
RAM, which lwIP, mbedTLS and every task stack compete for.

Do not add dirty-rectangle or partial-update logic. At one push per redraw the
full-frame cost is irrelevant and the complexity is not.

## Pages

| Page | Contents |
|------|----------|
| `PAGE_GLUCOSE` (0) | glucose value, trend arrow, delta, clock, battery, alarm bar |
| `PAGE_ERRORS` (1) | every held error (`STATUS_MAX_ERRORS == NS_ERR_LOG_SIZE`), total count |
| `PAGE_SYSTEM` (2) | free heap, uptime, IP, version, config errors |

`NUM_PAGES` is 3. Clock and log dates use `formatClock()` / `formatLogDate()` with
`time_format` and `date_format`; never hard-code `%02d:%02d`.

## Model / render split

`lib/ns_display_model/` turns state into a plain struct; `cores3/src/display.cpp`
only draws what the struct says. Layout decisions belong in the model so they
can be unit tested — put nothing in the renderer that a test would want to
assert.

## Staleness

`buildGlucoseModel()` classifies into `STALENESS_FRESH`, `STALENESS_STALE` and
`STALENESS_NO_DATA`. Stale greys the value and strikes it through; no-data
replaces it with `--.-` and raises a banner. An age that cannot be determined,
including a reading dated more than `SENSOR_FUTURE_TOLERANCE_SEC` ahead, is
no-data, never fresh. Stale starts after `SENSOR_AGE_STALE_MIN` (10) so normal
upload lag does not strike through a live feed. A sensor error code
(`sgvIsSensorError`) shows `--.-` with a `SENSOR ERROR` banner.

This is the most safety-relevant part of the UI. A stale reading that still
looks current is the failure mode that hurts someone.

## Touch

`M5.setTouchButtonHeight(40)` maps the bottom band to BtnA/B/C — left cycles
brightness, centre snoozes, right switches page. Use M5Unified's mapping; do not
hand-roll `setRawState()` calls.

## Trend arrows

`directionArrowStyle()` picks `ARROW_NONE`, `ARROW_SINGLE`, `ARROW_DOUBLE`
(Triple*) or `ARROW_RATE_OUT_OF_RANGE`. `buildGlucoseModel()` sets
`arrow_style` and forces `ARROW_NONE` for no-data and sensor-error states. Never
let an unrecognised direction share the rendering of a known one.

## Bottom band

The bottom 20 px holds the error badge, the touch labels, the alarm bar and the
battery. Positions are the `LAYOUT_*` constants in `ns_display_model.h`, and
`test_display_model` asserts the alarm bar never covers the badge or the
battery. `show_touch_labels` is false whenever the alarm bar is shown. The bar
always says what it means: `TAP TO SNOOZE` or `SNOOZED n min`, never a bare
number.
