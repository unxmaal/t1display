/*  ns_config_parse.cpp — INI config file parsing
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ns_config_parse.h"
#include "ns_pure_logic.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

/* ── Defaults ──────────────────────────────────────────────────── */

void configDefaults(ParsedConfig *cfg) {
    *cfg = ParsedConfig{};
    strlcpy(cfg->deviceName, "t1display", sizeof(cfg->deviceName));
    cfg->timeZone          = 3600;
    cfg->yellow_low        = 4.5f;
    cfg->yellow_high       = 9.0f;
    cfg->red_low           = 3.9f;
    cfg->red_high          = 11.0f;
    cfg->snd_alarm         = 3.0f;
    cfg->snd_warning       = 3.7f;
    cfg->snd_alarm_high    = 20.0f;
    cfg->snd_warning_high  = 14.0f;
    cfg->snd_no_readings   = 20;
    cfg->snooze_timeout    = 30;
    cfg->alarm_repeat      = 5;
    cfg->warning_volume    = 30;
    cfg->alarm_volume      = 100;
    cfg->brightness1       = 10;
    cfg->brightness2       = 50;
    cfg->brightness3       = 100;
    cfg->show_current_time = 1;
    cfg->info_line         = 1;
    cfg->snd_loop_error    = 1;
    strlcpy(cfg->restart_at_time, "NORES", sizeof(cfg->restart_at_time));
}

/* ── Helpers ───────────────────────────────────────────────────── */

static char *trimWhitespace(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}


static bool parseFloatIn(const char *val, float lo, float hi, float *out) {
    char *end;
    errno = 0;
    double d = strtod(val, &end);
    if (end == val || errno == ERANGE || !isfinite(d) || d < (double)lo || d > (double)hi)
        return false;
    *out = (float)d;
    return true;
}

static bool parseIntIn(const char *val, long lo, long hi, int *out) {
    char *end;
    errno = 0;
    long v = strtol(val, &end, 10);
    if (end == val || errno == ERANGE || v < lo || v > hi)
        return false;
    *out = (int)v;
    return true;
}

/* ── Parsing ───────────────────────────────────────────────────── */

int parseConfigBuffer(char *buf, size_t len, ParsedConfig *cfg) {
    int parsed = 0;
    int currentWlan = -1;  // -1 = [config] section, 0-9 = wlan index

    char *p = buf;
    const char *end = buf + len;

    while (p < end) {
        // Find end of line
        char *eol = p;
        while (eol < end && *eol != '\n' && *eol != '\r')
            eol++;

        // Null-terminate this line
        if (eol < end) *eol = '\0';

        char *line = trimWhitespace(p);

        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == ';' || line[0] == '#') {
            p = eol + 1;
            continue;
        }

        // Section header
        if (line[0] == '[') {
            char *close = strchr(line, ']');
            if (close) {
                *close = '\0';
                const char *section = line + 1;
                if (strcmp(section, "config") == 0) {
                    currentWlan = -1;
                } else if (strncmp(section, "wlan", 4) == 0) {
                    int idx = atoi(section + 4) - 1;  // wlan1 → 0
                    if (idx >= 0 && idx < CFG_MAX_WLAN)
                        currentWlan = idx;
                }
            }
            p = eol + 1;
            continue;
        }

        // Key = value
        char *eq = strchr(line, '=');
        if (!eq) {
            p = eol + 1;
            continue;
        }

        *eq = '\0';
        const char *key = trimWhitespace(line);
        const char *val = trimWhitespace(eq + 1);

        if (currentWlan >= 0 && currentWlan < CFG_MAX_WLAN) {
            // WiFi section
            if (strcmp(key, "ssid") == 0) {
                strlcpy(cfg->wlanssid[currentWlan], val, 64);
                parsed++;
            } else if (strcmp(key, "pass") == 0) {
                strlcpy(cfg->wlanpass[currentWlan], val, 64);
                parsed++;
            } else {
                cfg->unknownKeys++;
            }
        } else {
            // [config] section
            if (strcmp(key, "nightscout") == 0) {
                strlcpy(cfg->url, val, sizeof(cfg->url));
                parsed++;
            } else if (strcmp(key, "token") == 0) {
                strlcpy(cfg->token, val, sizeof(cfg->token));
                parsed++;
            } else if (strcmp(key, "name") == 0) {
                strlcpy(cfg->userName, val, sizeof(cfg->userName));
                parsed++;
            } else if (strcmp(key, "device_name") == 0) {
                strlcpy(cfg->deviceName, val, sizeof(cfg->deviceName));
                parsed++;
            } else if (strcmp(key, "time_zone") == 0) {
                if (parseIntIn(val, -43200, 50400, &cfg->timeZone)) parsed++;
            } else if (strcmp(key, "dst") == 0) {
                if (parseIntIn(val, 0, 7200, &cfg->dst)) parsed++;
            } else if (strcmp(key, "show_mgdl") == 0) {
                cfg->show_mgdl = atoi(val); parsed++;
            } else if (strcmp(key, "show_current_time") == 0) {
                cfg->show_current_time = atoi(val); parsed++;
            } else if (strcmp(key, "default_page") == 0) {
                cfg->default_page = atoi(val); parsed++;
            } else if (strcmp(key, "sgv_only") == 0) {
                cfg->sgv_only = atoi(val); parsed++;
            } else if (strcmp(key, "info_line") == 0) {
                cfg->info_line = atoi(val); parsed++;
            } else if (strcmp(key, "date_format") == 0) {
                cfg->date_format = atoi(val); parsed++;
            } else if (strcmp(key, "time_format") == 0) {
                if (parseIntIn(val, 0, 1, &cfg->time_format)) parsed++;
            } else if (strcmp(key, "yellow_low") == 0) {
                if (parseFloatIn(val, CFG_GLUCOSE_MIN, CFG_GLUCOSE_MAX, &cfg->yellow_low)) parsed++;
            } else if (strcmp(key, "yellow_high") == 0) {
                if (parseFloatIn(val, CFG_GLUCOSE_MIN, CFG_GLUCOSE_MAX, &cfg->yellow_high)) parsed++;
            } else if (strcmp(key, "red_low") == 0) {
                if (parseFloatIn(val, CFG_GLUCOSE_MIN, CFG_GLUCOSE_MAX, &cfg->red_low)) parsed++;
            } else if (strcmp(key, "red_high") == 0) {
                if (parseFloatIn(val, CFG_GLUCOSE_MIN, CFG_GLUCOSE_MAX, &cfg->red_high)) parsed++;
            } else if (strcmp(key, "snd_alarm") == 0) {
                if (parseFloatIn(val, CFG_GLUCOSE_MIN, CFG_GLUCOSE_MAX, &cfg->snd_alarm)) parsed++;
            } else if (strcmp(key, "snd_warning") == 0) {
                if (parseFloatIn(val, CFG_GLUCOSE_MIN, CFG_GLUCOSE_MAX, &cfg->snd_warning)) parsed++;
            } else if (strcmp(key, "snd_alarm_high") == 0) {
                if (parseFloatIn(val, CFG_GLUCOSE_MIN, CFG_GLUCOSE_MAX, &cfg->snd_alarm_high)) parsed++;
            } else if (strcmp(key, "snd_warning_high") == 0) {
                if (parseFloatIn(val, CFG_GLUCOSE_MIN, CFG_GLUCOSE_MAX, &cfg->snd_warning_high)) parsed++;
            } else if (strcmp(key, "snd_no_readings") == 0) {
                cfg->snd_no_readings = atoi(val); parsed++;
            } else if (strcmp(key, "snooze_timeout") == 0) {
                cfg->snooze_timeout = atoi(val); parsed++;
            } else if (strcmp(key, "alarm_repeat") == 0) {
                cfg->alarm_repeat = atoi(val); parsed++;
            } else if (strcmp(key, "warning_volume") == 0) {
                cfg->warning_volume = atoi(val); parsed++;
            } else if (strcmp(key, "alarm_volume") == 0) {
                cfg->alarm_volume = atoi(val); parsed++;
            } else if (strcmp(key, "brightness1") == 0) {
                cfg->brightness1 = atoi(val); parsed++;
            } else if (strcmp(key, "brightness2") == 0) {
                cfg->brightness2 = atoi(val); parsed++;
            } else if (strcmp(key, "brightness3") == 0) {
                cfg->brightness3 = atoi(val); parsed++;
            } else if (strcmp(key, "restart_at_logged_errors") == 0) {
                if (parseIntIn(val, 0, 1000, &cfg->restart_at_logged_errors)) parsed++;
            } else if (strcmp(key, "restart_at_time") == 0) {
                strlcpy(cfg->restart_at_time, val, sizeof(cfg->restart_at_time)); parsed++;
            } else if (strcmp(key, "snd_loop_error") == 0) {
                cfg->snd_loop_error = atoi(val); parsed++;
            } else {
                cfg->unknownKeys++;
            }
        }

        p = eol + 1;
    }

    validateConfig(cfg);
    return parsed;
}

static void clampGlucose(float *v, float fallback) {
    if (!isfinite(*v) || *v < CFG_GLUCOSE_MIN || *v > CFG_GLUCOSE_MAX)
        *v = fallback;
}

/* ── Validation ────────────────────────────────────────────────── */

void validateConfig(ParsedConfig *cfg) {
    clampGlucose(&cfg->yellow_low,       4.5f);
    clampGlucose(&cfg->yellow_high,      9.0f);
    clampGlucose(&cfg->red_low,          3.9f);
    clampGlucose(&cfg->red_high,        11.0f);
    clampGlucose(&cfg->snd_alarm,        3.0f);
    clampGlucose(&cfg->snd_warning,      3.7f);
    clampGlucose(&cfg->snd_alarm_high,  20.0f);
    clampGlucose(&cfg->snd_warning_high,14.0f);

    cfg->timeZone = clampInt(cfg->timeZone, -43200, 50400);
    cfg->dst      = clampInt(cfg->dst, 0, 7200);
    cfg->time_format = clampInt(cfg->time_format, 0, 1);
    cfg->restart_at_logged_errors = clampInt(cfg->restart_at_logged_errors, 0, 1000);

    cfg->show_mgdl         = clampInt(cfg->show_mgdl, 0, 1);
    cfg->show_current_time = clampInt(cfg->show_current_time, 0, 1);
    cfg->sgv_only          = clampInt(cfg->sgv_only, 0, 1);
    cfg->info_line         = clampInt(cfg->info_line, 0, 1);
    cfg->snd_loop_error    = clampInt(cfg->snd_loop_error, 0, 1);
    cfg->default_page      = clampInt(cfg->default_page, 0, 1);
    cfg->date_format       = clampInt(cfg->date_format, 0, 3);
    cfg->brightness1       = clampInt(cfg->brightness1, 0, 100);
    cfg->brightness2       = clampInt(cfg->brightness2, 0, 100);
    cfg->brightness3       = clampInt(cfg->brightness3, 0, 100);
    cfg->warning_volume    = clampInt(cfg->warning_volume, 0, 100);
    cfg->alarm_volume      = clampInt(cfg->alarm_volume, 0, 100);
    cfg->snooze_timeout    = clampInt(cfg->snooze_timeout, 0, 1440);
    cfg->alarm_repeat      = clampInt(cfg->alarm_repeat, 0, 1440);
    cfg->snd_no_readings   = clampInt(cfg->snd_no_readings, 0, 1440);
}

/* ── Serialization ─────────────────────────────────────────────── */

int serializeConfigINI(const ParsedConfig *cfg, char *buf, size_t bufSize) {
    if (!cfg || !buf || bufSize == 0) return 0;

    int pos = 0;
    int n;

#define EMIT(...) do { \
    n = snprintf(buf + pos, bufSize - (size_t)pos, __VA_ARGS__); \
    if (n < 0 || (size_t)(pos + n) >= bufSize) return 0; \
    pos += n; \
} while(0)

    EMIT("[config]\n");
    EMIT("nightscout = %s\n", cfg->url);
    EMIT("token = %s\n", cfg->token);
    EMIT("name = %s\n", cfg->userName);
    EMIT("device_name = %s\n", cfg->deviceName);
    EMIT("time_zone = %d\n", cfg->timeZone);
    EMIT("dst = %d\n", cfg->dst);
    EMIT("show_mgdl = %d\n", cfg->show_mgdl);
    EMIT("show_current_time = %d\n", cfg->show_current_time);
    EMIT("default_page = %d\n", cfg->default_page);
    EMIT("sgv_only = %d\n", cfg->sgv_only);
    EMIT("info_line = %d\n", cfg->info_line);
    EMIT("date_format = %d\n", cfg->date_format);
    EMIT("time_format = %d\n", cfg->time_format);
    EMIT("yellow_low = %.1f\n", (double)cfg->yellow_low);
    EMIT("yellow_high = %.1f\n", (double)cfg->yellow_high);
    EMIT("red_low = %.1f\n", (double)cfg->red_low);
    EMIT("red_high = %.1f\n", (double)cfg->red_high);
    EMIT("snd_alarm = %.1f\n", (double)cfg->snd_alarm);
    EMIT("snd_warning = %.1f\n", (double)cfg->snd_warning);
    EMIT("snd_alarm_high = %.1f\n", (double)cfg->snd_alarm_high);
    EMIT("snd_warning_high = %.1f\n", (double)cfg->snd_warning_high);
    EMIT("snd_no_readings = %d\n", cfg->snd_no_readings);
    EMIT("snooze_timeout = %d\n", cfg->snooze_timeout);
    EMIT("alarm_repeat = %d\n", cfg->alarm_repeat);
    EMIT("warning_volume = %d\n", cfg->warning_volume);
    EMIT("alarm_volume = %d\n", cfg->alarm_volume);
    EMIT("brightness1 = %d\n", cfg->brightness1);
    EMIT("brightness2 = %d\n", cfg->brightness2);
    EMIT("brightness3 = %d\n", cfg->brightness3);
    EMIT("restart_at_logged_errors = %d\n", cfg->restart_at_logged_errors);
    EMIT("restart_at_time = %s\n", cfg->restart_at_time);
    EMIT("snd_loop_error = %d\n", cfg->snd_loop_error);

    for (int i = 0; i < CFG_MAX_WLAN; i++) {
        if (cfg->wlanssid[i][0] != '\0') {
            EMIT("\n[wlan%d]\n", i + 1);
            EMIT("ssid = %s\n", cfg->wlanssid[i]);
            EMIT("pass = %s\n", cfg->wlanpass[i]);
        }
    }

#undef EMIT

    buf[pos] = '\0';
    return pos;
}
