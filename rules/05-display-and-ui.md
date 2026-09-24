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
| `PAGE_STATUS` (1) | error log, free heap, uptime, IP, version |

`NUM_PAGES` is 2.

## Model / render split

`lib/ns_display_model/` turns state into a plain struct; `cores3/src/display.cpp`
only draws what the struct says. Layout decisions belong in the model so they
can be unit tested — put nothing in the renderer that a test would want to
assert.

## Staleness

`buildGlucoseModel()` classifies into `STALENESS_FRESH`, `STALENESS_STALE` and
`STALENESS_NO_DATA`. Stale greys the value and strikes it through; no-data
replaces it with `--.-` and raises a banner. An age that cannot be determined is
no-data, never fresh.

This is the most safety-relevant part of the UI. A stale reading that still
looks current is the failure mode that hurts someone.

## Touch

`M5.setTouchButtonHeight(40)` maps the bottom band to BtnA/B/C — left cycles
brightness, centre snoozes, right switches page. Use M5Unified's mapping; do not
hand-roll `setRawState()` calls.
