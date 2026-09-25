/*  ns_pure_logic.cpp — Hardware-free pure logic for t1display
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ns_pure_logic.h"
#include <string.h>
#include <stdio.h>

/* ── Integer clamping ──────────────────────────────────────────── */

int clampInt(int value, int min_val, int max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

/* ── Direction → angle mapping ──────────────────────────────────── */

int directionToAngle(const char* direction) {
    if (direction == NULL)
        return 180;

    if (strcmp(direction, "DoubleDown") == 0 || strcmp(direction, "DOUBLE_DOWN") == 0)
        return 90;
    if (strcmp(direction, "SingleDown") == 0 || strcmp(direction, "SINGLE_DOWN") == 0)
        return 75;
    if (strcmp(direction, "FortyFiveDown") == 0 || strcmp(direction, "FORTY_FIVE_DOWN") == 0)
        return 45;
    if (strcmp(direction, "Flat") == 0 || strcmp(direction, "FLAT") == 0)
        return 0;
    if (strcmp(direction, "FortyFiveUp") == 0 || strcmp(direction, "FORTY_FIVE_UP") == 0)
        return -45;
    if (strcmp(direction, "SingleUp") == 0 || strcmp(direction, "SINGLE_UP") == 0)
        return -75;
    if (strcmp(direction, "DoubleUp") == 0 || strcmp(direction, "DOUBLE_UP") == 0)
        return -90;
    if (strcmp(direction, "TripleUp") == 0 || strcmp(direction, "TRIPLE_UP") == 0)
        return -90;
    if (strcmp(direction, "TripleDown") == 0 || strcmp(direction, "TRIPLE_DOWN") == 0)
        return 90;

    return 180;  // NONE, NOT COMPUTABLE, unknown
}

int directionArrowStyle(const char* direction) {
    if (direction == NULL)
        return ARROW_NONE;
    if (strcmp(direction, "RATE OUT OF RANGE") == 0 || strcmp(direction, "RATE_OUT_OF_RANGE") == 0)
        return ARROW_RATE_OUT_OF_RANGE;
    if (strncmp(direction, "Triple", 6) == 0 || strncmp(direction, "TRIPLE_", 7) == 0)
        return ARROW_DOUBLE;
    return directionToAngle(direction) == 180 ? ARROW_NONE : ARROW_SINGLE;
}

/* ── JSON sanitization ──────────────────────────────────────────── */

static size_t replace6WithSpace(char* buf, size_t len, size_t pos) {
    buf[pos] = ' ';
    size_t tail = len - (pos + 6);
    memmove(&buf[pos + 1], &buf[pos + 6], tail + 1);  // +1 for null
    return len - 5;
}

size_t sanitizeJson(char* buf, size_t len) {
    // 1. Replace control characters (< 32) with space
    for (size_t i = 0; i < len; i++) {
        if ((unsigned char)buf[i] < 32)
            buf[i] = ' ';
    }

    // 2. Replace problematic unicode escape sequences
    const char* escapes[] = { "\\u0000", "\\u000b", "\\u0002" };
    for (int e = 0; e < 3; e++) {
        size_t i = 0;
        while (i + 5 < len) {
            if (strncmp(&buf[i], escapes[e], 6) == 0) {
                len = replace6WithSpace(buf, len, i);
                // don't advance i — check same position again
            } else {
                i++;
            }
        }
    }

    // 3. Strip fractional milliseconds from "date": fields
    const char* dateKey = "\"date\":";
    size_t dkLen = strlen(dateKey);
    size_t i = 0;
    while (i + dkLen < len) {
        if (strncmp(&buf[i], dateKey, dkLen) == 0) {
            // Find the dot after digits
            size_t numStart = i + dkLen;
            size_t j = numStart;
            // skip digits
            while (j < len && buf[j] >= '0' && buf[j] <= '9')
                j++;
            // if dot follows, strip dot and trailing digits
            if (j < len && buf[j] == '.') {
                size_t dotPos = j;
                j++;
                while (j < len && buf[j] >= '0' && buf[j] <= '9')
                    j++;
                // remove from dotPos to j-1
                size_t removeCount = j - dotPos;
                memmove(&buf[dotPos], &buf[j], len - j + 1);  // +1 for null
                len -= removeCount;
            }
            i = numStart;
        } else {
            i++;
        }
    }

    return len;
}

/* ── Sensor age ────────────────────────────────────────────────── */

int sensorAgeMinutes(long now_sec, long sensor_sec) {
    if (now_sec <= 0 || sensor_sec <= 0)
        return SENSOR_AGE_UNKNOWN;
    long age_sec = now_sec - sensor_sec;
    if (age_sec < -SENSOR_FUTURE_TOLERANCE_SEC)
        return SENSOR_AGE_UNKNOWN;
    if (age_sec < 0)
        return 0;
    long mins = (age_sec + 30) / 60;
    return (mins > SENSOR_AGE_UNKNOWN) ? SENSOR_AGE_UNKNOWN : (int)mins;
}

bool sgvIsSensorError(float sgv_mmol) {
    return !(sgv_mmol >= SGV_MIN_VALID_MGDL / MGDL_PER_MMOL);
}

/* ── Glucose color level ───────────────────────────────────────── */

int glucoseColor(float sgv, float yellow_low, float yellow_high,
                 float red_low, float red_high) {
    int color = GLUCOSE_COLOR_GREEN;
    if (sgv < yellow_low || sgv > yellow_high)
        color = GLUCOSE_COLOR_YELLOW;
    if (sgv < red_low || sgv > red_high)
        color = GLUCOSE_COLOR_RED;
    return color;
}

/* ── Alarm level decision ──────────────────────────────────────── */

int alarmLevel(float sgv, float snd_alarm, float snd_warning,
               float snd_alarm_high, float snd_warning_high,
               unsigned int sensor_age_min, unsigned int snd_no_readings,
               bool has_loop_error) {
    /* Priority chain: low alarm > low warn > high alarm > high warn > no readings > loop error */
    if (sgvIsSensorError(sgv))
        return ALARM_LEVEL_NO_READINGS;

    if (sgv <= snd_alarm)
        return ALARM_LEVEL_LOW_ALARM;
    if (sgv <= snd_warning)
        return ALARM_LEVEL_LOW_WARNING;
    if (sgv >= snd_alarm_high)
        return ALARM_LEVEL_HIGH_ALARM;
    if (sgv >= snd_warning_high)
        return ALARM_LEVEL_HIGH_WARNING;
    if (sensor_age_min >= snd_no_readings)
        return ALARM_LEVEL_NO_READINGS;
    if (has_loop_error)
        return ALARM_LEVEL_LOOP_ERROR;
    return ALARM_LEVEL_NORMAL;
}

/* ── Glucose string formatting ─────────────────────────────────── */

int formatGlucose(char *buf, size_t bufsize,
                  float sgv_mmol, float sgv_mgdl, bool show_mgdl) {
    if (show_mgdl) {
        snprintf(buf, bufsize, "%.0f", sgv_mgdl);
        return FONT_LARGE;
    }
    /* mmol/L */
    if (sgv_mmol < 10.0f) {
        snprintf(buf, bufsize, "%.1f", sgv_mmol);
        return FONT_LARGE;
    }
    snprintf(buf, bufsize, "%.1f", sgv_mmol);
    return FONT_MEDIUM;
}

/* ── Uptime formatting ─────────────────────────────────────────── */

void formatUptime(char *buf, size_t bufsize, unsigned long ms) {
    unsigned long total_sec = ms / 1000;
    int days = (int)(total_sec / 86400);
    int rem = (int)(total_sec % 86400);
    int hours = rem / 3600;
    rem %= 3600;
    int minutes = rem / 60;
    int seconds = rem % 60;
    snprintf(buf, bufsize, "%02dd %02d:%02d:%02d", days, hours, minutes, seconds);
}
