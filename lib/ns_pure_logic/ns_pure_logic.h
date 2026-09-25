/*  ns_pure_logic.h — Hardware-free pure logic for t1display
 *
 *  Every function here compiles on both ESP32 (Arduino) and host (native).
 *  No Arduino.h, no M5Stack.h, no WiFi.h — only standard C/C++ types.
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NS_PURE_LOGIC_H
#define NS_PURE_LOGIC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Integer clamping ──────────────────────────────────────────── */

int clampInt(int value, int min_val, int max_val);

/* ── Direction → angle mapping ──────────────────────────────────── */

/**
 * Convert a Nightscout direction string to a display angle (degrees).
 *   "DoubleUp"   / "DOUBLE_UP"       → -90
 *   "SingleUp"   / "SINGLE_UP"       → -75
 *   "FortyFiveUp"/ "FORTY_FIVE_UP"   → -45
 *   "Flat"       / "FLAT"            →   0
 *   "FortyFiveDown"/"FORTY_FIVE_DOWN"→  45
 *   "SingleDown" / "SINGLE_DOWN"     →  75
 *   "DoubleDown" / "DOUBLE_DOWN"     →  90
 *   "TripleUp"   / "TRIPLE_UP"       → -90
 *   "TripleDown" / "TRIPLE_DOWN"     →  90
 *   anything else (incl. "NONE", "NOT COMPUTABLE", NULL) → 180
 */
int directionToAngle(const char* direction);

#define ARROW_NONE              0
#define ARROW_SINGLE            1
#define ARROW_DOUBLE            2
#define ARROW_RATE_OUT_OF_RANGE 3

/** How to draw the trend: none, one arrow, two arrows (Triple*), or the
 *  rate-out-of-range glyph. */
int directionArrowStyle(const char* direction);

/* ── JSON sanitization ──────────────────────────────────────────── */

/**
 * Sanitize a JSON buffer in-place:
 *   1. Replace chars < 32 with space
 *   2. Replace \u0000, \u000b, \u0002 with spaces
 *   3. Strip fractional milliseconds from "date": fields
 *      (e.g. "date":1234567890.123 → "date":1234567890)
 *
 * buf must be a writable, null-terminated C string.
 * Returns the new length (may be shorter due to removals).
 */
size_t sanitizeJson(char* buf, size_t len);

/* ── Sensor age ────────────────────────────────────────────────── */

#define SENSOR_AGE_UNKNOWN 1440
#define SENSOR_FUTURE_TOLERANCE_SEC 120

/**
 * Minutes between a reading's timestamp and now, rounded to nearest.
 * Returns SENSOR_AGE_UNKNOWN when either timestamp is unavailable or the
 * reading is dated more than SENSOR_FUTURE_TOLERANCE_SEC ahead, and 0 for
 * smaller future skew.
 */
int sensorAgeMinutes(long now_sec, long sensor_sec);

/* ── Units and sensor error codes ──────────────────────────────── */

#define MGDL_PER_MMOL 18.01559f
#define SGV_MIN_VALID_MGDL 39.0f

/**
 * True when an sgv (mmol/L) is a CGM error code rather than a reading.
 * Dexcom reports sensor states as values below 39 mg/dL.
 */
bool sgvIsSensorError(float sgv_mmol);

/* ── Glucose color level ───────────────────────────────────────── */

#define GLUCOSE_COLOR_GREEN  0
#define GLUCOSE_COLOR_YELLOW 1
#define GLUCOSE_COLOR_RED    2

/**
 * Determine display color for a glucose value given warning/alert thresholds.
 * Returns GLUCOSE_COLOR_GREEN, GLUCOSE_COLOR_YELLOW, or GLUCOSE_COLOR_RED.
 *
 * Logic matches the .ino: yellow if outside [yellow_low..yellow_high],
 * red overrides if outside [red_low..red_high].
 */
int glucoseColor(float sgv, float yellow_low, float yellow_high,
                 float red_low, float red_high);

/* ── Alarm level decision ──────────────────────────────────────── */

#define ALARM_LEVEL_NORMAL       0
#define ALARM_LEVEL_LOW_ALARM    1
#define ALARM_LEVEL_LOW_WARNING  2
#define ALARM_LEVEL_HIGH_ALARM   3
#define ALARM_LEVEL_HIGH_WARNING 4
#define ALARM_LEVEL_NO_READINGS  5
#define ALARM_LEVEL_LOOP_ERROR   6

/**
 * Determine the alarm level from glucose value + config thresholds.
 * Mirrors the priority chain in handleAlarmsInfoLine():
 *   0. No readings   (sgvIsSensorError)
 *   1. Low alarm   (sgv <= snd_alarm)
 *   2. Low warning  (sgv <= snd_warning)
 *   3. High alarm   (sgv >= snd_alarm_high)
 *   4. High warning  (sgv >= snd_warning_high)
 *   5. No readings   (sensor_age_min >= snd_no_readings)
 *   6. Loop error    (has_loop_error)
 *   7. Normal
 */
int alarmLevel(float sgv, float snd_alarm, float snd_warning,
               float snd_alarm_high, float snd_warning_high,
               unsigned int sensor_age_min, unsigned int snd_no_readings,
               bool has_loop_error);

/* ── Glucose string formatting ─────────────────────────────────── */

#define FONT_LARGE  0
#define FONT_MEDIUM 1

/**
 * Format a glucose value into a display string.
 *   show_mgdl=true  → use sgv_mgdl, integer format ("120")
 *   show_mgdl=false → use sgv_mmol, one decimal ("8.4" or "12.3")
 * Returns a font hint: FONT_LARGE or FONT_MEDIUM.
 */
int formatGlucose(char *buf, size_t bufsize,
                  float sgv_mmol, float sgv_mgdl, bool show_mgdl);

/* ── Uptime formatting ─────────────────────────────────────────── */

/**
 * Format milliseconds into "DDd HH:MM:SS" string.
 */
void formatUptime(char *buf, size_t bufsize, unsigned long millis);

#ifdef __cplusplus
}
#endif

#endif /* NS_PURE_LOGIC_H */
