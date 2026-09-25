/*  ns_display_model.cpp — Display model builders
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ns_display_model.h"
#include "ns_pure_logic.h"
#include <string.h>
#include <stdio.h>

/* ── Error code → description ──────────────────────────────────── */

void describeErrorCode(int code, char *buf, size_t bufsize) {
    switch (code) {
        case 1001: strlcpy(buf, "JSON parse failed", bufsize); break;
        case 1002: strlcpy(buf, "No data from NS", bufsize); break;
        case 1003: strlcpy(buf, "JSON2 parse failed", bufsize); break;
        default:
            if (code < 0)
                snprintf(buf, bufsize, "HTTP err %d", code);
            else
                snprintf(buf, bufsize, "HTTP %d", code);
            break;
    }
}

/* ── Alarm bar colors ──────────────────────────────────────────── */

static void alarmBarColors(int alarm_level, int *bg, int *fg) {
    switch (alarm_level) {
        case ALARM_LEVEL_LOW_ALARM:
        case ALARM_LEVEL_HIGH_ALARM:
        case ALARM_LEVEL_LOOP_ERROR:
            *bg = COLOR_RED;
            *fg = COLOR_BLACK;
            break;
        case ALARM_LEVEL_LOW_WARNING:
        case ALARM_LEVEL_HIGH_WARNING:
        case ALARM_LEVEL_NO_READINGS:
            *bg = COLOR_YELLOW;
            *fg = COLOR_BLACK;
            break;
        default:
            *bg = COLOR_BLACK;
            *fg = COLOR_LIGHTGREY;
            break;
    }
}

/* ── Glucose page ──────────────────────────────────────────────── */

void buildGlucoseModel(
    GlucosePageModel *model,
    float sgv_mmol, float sgv_mgdl, bool show_mgdl,
    const char *direction, int arrow_angle,
    const char *delta_display,
    int time_hour, int time_min,
    long now_sec, long sensor_time_sec,
    float yellow_low, float yellow_high,
    float red_low, float red_high,
    int alarm_level, int snooze_remaining_sec,
    int battery_pct, int error_count,
    int time_format
) {
    memset(model, 0, sizeof(*model));

    /* Time */
    formatClock(model->time_str, sizeof(model->time_str), time_hour, time_min, time_format);

    /* Glucose */
    model->glucose_font = formatGlucose(model->glucose_str,
                                         sizeof(model->glucose_str),
                                         sgv_mmol, sgv_mgdl, show_mgdl);
    /* Map font hint to model font */
    model->glucose_font = (model->glucose_font == FONT_MEDIUM)
                          ? FONT_SANS_BOLD_18 : FONT_SANS_BOLD_24;

    int cl = glucoseColor(sgv_mmol, yellow_low, yellow_high, red_low, red_high);
    switch (cl) {
        case GLUCOSE_COLOR_YELLOW: model->glucose_color = COLOR_YELLOW; break;
        case GLUCOSE_COLOR_RED:    model->glucose_color = COLOR_RED; break;
        default:                   model->glucose_color = COLOR_GREEN; break;
    }

    /* Delta */
    strlcpy(model->delta_str, delta_display ? delta_display : "", sizeof(model->delta_str));

    /* Arrow */
    model->arrow_angle = arrow_angle;
    model->arrow_style = directionArrowStyle(direction);
    model->arrow_color = model->glucose_color;

    /* Sensor staleness */
    int age_min = sensorAgeMinutes(now_sec, sensor_time_sec);

    if (age_min >= SENSOR_AGE_NO_DATA_MIN)
        model->staleness = STALENESS_NO_DATA;
    else if (age_min > SENSOR_AGE_STALE_MIN)
        model->staleness = STALENESS_STALE;
    else
        model->staleness = STALENESS_FRESH;

    if (model->staleness != STALENESS_FRESH) {
        model->show_age = true;
        snprintf(model->age_str, sizeof(model->age_str), "%d min", age_min);
        model->age_color = (age_min > SENSOR_AGE_CRITICAL_MIN) ? COLOR_RED : COLOR_WHITE;
    }

    if (model->staleness == STALENESS_NO_DATA) {
        strlcpy(model->last_seen_str, model->glucose_str, sizeof(model->last_seen_str));
        strlcpy(model->glucose_str, "--.-", sizeof(model->glucose_str));
        model->glucose_color = COLOR_LIGHTGREY;
        model->arrow_angle   = 180;
        model->arrow_style   = ARROW_NONE;
        model->show_banner   = true;
        snprintf(model->banner_str, sizeof(model->banner_str), "NO DATA %d min", age_min);
    } else if (sgvIsSensorError(sgv_mmol)) {
        strlcpy(model->glucose_str, "--.-", sizeof(model->glucose_str));
        model->glucose_color = COLOR_LIGHTGREY;
        model->arrow_angle   = 180;
        model->arrow_style   = ARROW_NONE;
        model->show_banner   = true;
        strlcpy(model->banner_str, "SENSOR ERROR", sizeof(model->banner_str));
    } else if (model->staleness == STALENESS_STALE) {
        model->strike_glucose = true;
        model->glucose_color  = COLOR_LIGHTGREY;
        model->arrow_color    = COLOR_LIGHTGREY;
    }

    /* Battery */
    model->battery_pct = battery_pct;

    /* Error badge */
    model->show_error_badge = (error_count > 0);

    /* Alarm bar */
    int bar_bg, bar_fg;
    alarmBarColors(alarm_level, &bar_bg, &bar_fg);
    model->alarm_bar_bg = bar_bg;
    model->alarm_bar_fg = bar_fg;
    model->show_alarm_bar = (alarm_level != ALARM_LEVEL_NORMAL || snooze_remaining_sec > 0);
    if (snooze_remaining_sec > 0) {
        snprintf(model->alarm_bar_text, sizeof(model->alarm_bar_text),
                 "SNOOZED %d min", (snooze_remaining_sec + 59) / 60);
    } else if (alarm_level != ALARM_LEVEL_NORMAL) {
        strlcpy(model->alarm_bar_text, "TAP TO SNOOZE", sizeof(model->alarm_bar_text));
    }
    model->show_touch_labels = !model->show_alarm_bar;
}

/* ── Status page ───────────────────────────────────────────────── */

void buildStatusModel(
    StatusPageModel *model,
    const int *err_codes, const char (*err_dates)[16],
    int err_display_count, int err_total_count,
    unsigned long heap_free, unsigned long uptime_ms,
    const char *ip_str, const char *version,
    int battery_pct
) {
    memset(model, 0, sizeof(*model));

    model->error_count = err_total_count;
    model->display_count = err_display_count;
    if (model->display_count > STATUS_MAX_ERRORS)
        model->display_count = STATUS_MAX_ERRORS;
    if (err_codes == NULL || err_dates == NULL)
        model->display_count = 0;

    for (int i = 0; i < model->display_count; i++) {
        strlcpy(model->errors[i].date_str, err_dates[i],
                sizeof(model->errors[i].date_str));
        describeErrorCode(err_codes[i], model->errors[i].desc_str,
                          sizeof(model->errors[i].desc_str));
        model->errors[i].color = (err_codes[i] < 0) ? COLOR_RED : COLOR_YELLOW;
    }

    snprintf(model->heap_str, sizeof(model->heap_str),
             "Free Heap = %lu", heap_free);

    char uptimeBuf[22];
    formatUptime(uptimeBuf, sizeof(uptimeBuf), uptime_ms);
    snprintf(model->uptime_str, sizeof(model->uptime_str),
             "Up time = %s", uptimeBuf);

    strlcpy(model->ip_str, ip_str ? ip_str : "0.0.0.0", sizeof(model->ip_str));
    strlcpy(model->version_str, version ? version : "?", sizeof(model->version_str));
    model->battery_pct = battery_pct;
}
