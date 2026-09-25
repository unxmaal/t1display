/*  ns_display_model.h — Display model: what to draw, not how.
 *
 *  Pure data structs describing screen contents. No hardware deps.
 *  A thin renderer reads these and calls M5.Display on the device.
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NS_DISPLAY_MODEL_H
#define NS_DISPLAY_MODEL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ── Sensor age thresholds (minutes) ───────────────────────────── */

#define SENSOR_AGE_NO_DATA_MIN   20
#define SENSOR_AGE_STALE_MIN     10
#define SENSOR_AGE_CRITICAL_MIN  15

/* ── Color level (maps to TFT colors in the renderer) ──────────── */

#define COLOR_BLACK     0
#define COLOR_GREEN     1
#define COLOR_YELLOW    2
#define COLOR_RED       3
#define COLOR_WHITE     4
#define COLOR_LIGHTGREY 5

#define STALENESS_FRESH   0
#define STALENESS_STALE   1
#define STALENESS_NO_DATA 2

/* ── Font hints (maps to actual fonts in the renderer) ─────────── */

#define FONT_SANS_BOLD_24  0
#define FONT_SANS_BOLD_18  1
#define FONT_SANS_BOLD_12  2
#define FONT_SANS_9        3
#define FONT_MONO_9        4

/* ── Bottom band layout (pixels) ───────────────────────────────── */

#define LAYOUT_SCREEN_W      320
#define LAYOUT_BAND_Y        220
#define LAYOUT_BAND_H        20
#define LAYOUT_BADGE_X       2
#define LAYOUT_BADGE_W       10
#define LAYOUT_BATTERY_X     296
#define LAYOUT_BATTERY_W     22
#define LAYOUT_ALARM_BAR_X   14
#define LAYOUT_ALARM_BAR_W   280
#define LAYOUT_LABEL_LEFT_X  53
#define LAYOUT_LABEL_RIGHT_X 250

/* ── Glucose page model ────────────────────────────────────────── */

struct GlucosePageModel {
    /* Time display */
    char time_str[16];

    /* Glucose */
    char glucose_str[16];
    int  glucose_color;       // COLOR_GREEN/YELLOW/RED
    int  glucose_font;        // FONT_SANS_BOLD_24 or _18

    /* Delta */
    char delta_str[16];

    /* Trend arrow */
    int  arrow_angle;         // degrees, 180 = hidden
    int  arrow_style;         // ARROW_*
    int  arrow_color;         // same as glucose_color

    /* Sensor staleness */
    int  staleness;           // STALENESS_FRESH/STALE/NO_DATA
    bool strike_glucose;
    bool show_age;
    char age_str[16];
    int  age_color;           // COLOR_WHITE or COLOR_RED
    bool show_banner;
    char banner_str[32];
    char last_seen_str[32];

    /* Battery */
    int  battery_pct;         // 0-100, -1 = unknown

    /* Error indicator */
    bool show_error_badge;

    /* Alarm bar */
    bool show_alarm_bar;
    int  alarm_bar_bg;        // COLOR_RED, COLOR_YELLOW, or COLOR_BLACK
    int  alarm_bar_fg;        // foreground text color
    char alarm_bar_text[16];  // snooze countdown or empty

    /* Touch zone labels */
    bool show_touch_labels;
};

/* ── Status page model ─────────────────────────────────────────── */

#define STATUS_MAX_ERRORS 10

struct StatusErrorEntry {
    char date_str[16];    // formatLogDate()
    char desc_str[32];    // error description
    int  color;           // COLOR_RED or COLOR_YELLOW
};

struct StatusPageModel {
    /* Errors */
    int  error_count;     // total since boot
    int  display_count;   // how many to show
    StatusErrorEntry errors[STATUS_MAX_ERRORS];

    /* System info */
    char heap_str[32];
    char uptime_str[32];
    char ip_str[32];
    char version_str[16];

    /* Battery */
    int  battery_pct;
};

/* ── Model builders (pure functions, no hardware) ──────────────── */

#ifdef __cplusplus
extern "C" {
#endif

/** Human-readable text for an error-log code: app codes, HTTP statuses, HTTPClient errors. */
void describeErrorCode(int code, char *buf, size_t bufsize);

/**
 * Build a GlucosePageModel from raw data.
 * All display decisions happen here — the renderer just draws.
 *
 * now_sec: current time as seconds since epoch (0 = unknown)
 * battery_pct: 0-100, or -1 if unknown
 */
void buildGlucoseModel(
    GlucosePageModel *model,
    /* glucose data */
    float sgv_mmol, float sgv_mgdl, bool show_mgdl,
    const char *direction, int arrow_angle,
    const char *delta_display,
    /* time */
    int time_hour, int time_min,
    /* sensor staleness */
    long now_sec, long sensor_time_sec,
    /* thresholds */
    float yellow_low, float yellow_high,
    float red_low, float red_high,
    /* alarm state */
    int alarm_level, int snooze_remaining_sec,
    /* status */
    int battery_pct, int error_count,
    int time_format
);

/**
 * Build a StatusPageModel from error log + system info.
 */
void buildStatusModel(
    StatusPageModel *model,
    /* errors — pass arrays of codes and times */
    const int *err_codes, const char (*err_dates)[16],
    int err_display_count, int err_total_count,
    /* system */
    unsigned long heap_free, unsigned long uptime_ms,
    const char *ip_str, const char *version,
    int battery_pct
);

#ifdef __cplusplus
}
#endif

#endif // NS_DISPLAY_MODEL_H
