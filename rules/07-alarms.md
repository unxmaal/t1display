# Rule 07: Alarms and Audio

## Alarm Levels

Thresholds are always mmol/L. See `rules/04-configuration.md`.

| Level | Config key | Default | Volume |
|-------|-----------|---------|--------|
| Low alarm | `snd_alarm` | 3.0 | `alarm_volume` |
| Low warning | `snd_warning` | 3.7 | `warning_volume` |
| High warning | `snd_warning_high` | 14.0 | `warning_volume` |
| High alarm | `snd_alarm_high` | 20.0 | `alarm_volume` |
| No readings | `snd_no_readings` | 20 min | `warning_volume` |

Level selection is `alarmLevel()` in `lib/ns_pure_logic/`. An sgv below
`SGV_MIN_VALID_MGDL` (39 mg/dL) is a CGM error code, not a hypo, and returns
`ALARM_LEVEL_NO_READINGS`. 39 itself is Dexcom's LOW and must alarm. All unit
conversion uses `MGDL_PER_MMOL`.

## Audio

Single path: `M5.Speaker` (I2S) on the CoreS3. There is no DAC path, no LED
strip and no vibration motor in this build.

Melodies are sequenced non-blocking by `lib/ns_melody/`. `playMelody()` only
arms the sequencer; `serviceAlerts()` emits one note per loop iteration as each
becomes due. Never reintroduce `delay()` here — it makes the snooze button
unresponsive for the duration of the alarm it is meant to silence.

Note and duration tables passed to `melodyStart()` must be `static`. The
sequencer holds pointers to them well after the starting call returns.

## Scheduling

`lib/ns_alarm_state/` owns repeat and snooze timing, driven by `millis()`.
Nothing in the alarm path reads the wall clock, so alarms work on a device that
never synced NTP.

- Repeat interval is `alarm_repeat` minutes since the last fire.
- Snooze records the level that was snoozed and covers only the same direction
  (low, high, or data: no readings and loop error) at equal or lesser severity.
  A condition in another direction, or a more severe one, always sounds.
- A press for the same level within `ALARM_SNOOZE_DEBOUNCE_MS` is ignored. A
  later press adds one `snooze_timeout`, capped at `ALARM_SNOOZE_MAX_SEC`.
- `snooze_timeout` is at least 1 minute, so the button can never do nothing.
- `alarmSound()` maps a level to its melody. Loop error uses the no-readings
  melody, never a glucose alarm's.
- Snooze does not survive a reboot. There is no NVS and no UDP sync.

The remaining snooze time is passed into `drawPage()` and rendered in the alarm
bar. A silenced alarm must always say so on screen.
