/*  ns_config_parse.cpp — INI config file parsing
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ns_config_parse.h"
#include "ns_pure_logic.h"
#include "ns_restart_schedule.h"
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

/* ── Defaults ──────────────────────────────────────────────────── */

static void defaultBands(ParsedConfig *cfg) {
    cfg->yellow_low  = 4.5f;
    cfg->yellow_high = 9.0f;
    cfg->red_low     = 3.9f;
    cfg->red_high    = 11.0f;
}

static void defaultAlarmThresholds(ParsedConfig *cfg) {
    cfg->snd_alarm        = 3.0f;
    cfg->snd_warning      = 3.7f;
    cfg->snd_alarm_high   = 20.0f;
    cfg->snd_warning_high = 14.0f;
}

void configDefaults(ParsedConfig *cfg) {
    *cfg = ParsedConfig{};
    strlcpy(cfg->deviceName, "t1display", sizeof(cfg->deviceName));
    defaultBands(cfg);
    defaultAlarmThresholds(cfg);
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
    if (end == val || errno == ERANGE)
        return false;
    bool inRange = v >= lo && v <= hi;
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    *out = (int)v;
    return inRange;
}

static void configError(ParsedConfig *cfg, const char *what) {
    if (cfg->configErrors++ == 0)
        strlcpy(cfg->firstBadKey, what, sizeof(cfg->firstBadKey));
}

static void copyValue(ParsedConfig *cfg, const char *key, char *dst,
                      size_t size, const char *val) {
    if (strlcpy(dst, val, size) >= size)
        configError(cfg, key);
}

/* ── Parsing ───────────────────────────────────────────────────── */

static bool applyGlucoseKey(ParsedConfig *cfg, const char *key, const char *val,
                            int mgdl) {
    float *dst = NULL;
    if      (strcmp(key, "yellow_low")       == 0) dst = &cfg->yellow_low;
    else if (strcmp(key, "yellow_high")      == 0) dst = &cfg->yellow_high;
    else if (strcmp(key, "red_low")          == 0) dst = &cfg->red_low;
    else if (strcmp(key, "red_high")         == 0) dst = &cfg->red_high;
    else if (strcmp(key, "snd_alarm")        == 0) dst = &cfg->snd_alarm;
    else if (strcmp(key, "snd_warning")      == 0) dst = &cfg->snd_warning;
    else if (strcmp(key, "snd_alarm_high")   == 0) dst = &cfg->snd_alarm_high;
    else if (strcmp(key, "snd_warning_high") == 0) dst = &cfg->snd_warning_high;
    if (!dst) return false;

    float lo = mgdl ? CFG_GLUCOSE_MIN * MGDL_PER_MMOL : CFG_GLUCOSE_MIN;
    float hi = mgdl ? CFG_GLUCOSE_MAX * MGDL_PER_MMOL : CFG_GLUCOSE_MAX;
    float raw;
    if (parseFloatIn(val, lo, hi, &raw))
        *dst = mgdl ? raw / MGDL_PER_MMOL : raw;
    else if (!mgdl && parseFloatIn(val, CFG_GLUCOSE_MAX, CFG_GLUCOSE_MAX * MGDL_PER_MMOL, &raw))
        *dst = raw / MGDL_PER_MMOL;
    else
        configError(cfg, key);
    return true;
}

static bool applyIntKey(ParsedConfig *cfg, const char *key, const char *val) {
    struct { const char *name; int *dst; long lo; long hi; } fields[] = {
        { "time_zone",                &cfg->timeZone,                -43200, 50400 },
        { "dst",                      &cfg->dst,                          0,  7200 },
        { "show_mgdl",                &cfg->show_mgdl,                    0,     1 },
        { "show_current_time",        &cfg->show_current_time,            0,     1 },
        { "default_page",             &cfg->default_page,                 0,     2 },
        { "sgv_only",                 &cfg->sgv_only,                     0,     1 },
        { "info_line",                &cfg->info_line,                    0,     1 },
        { "date_format",              &cfg->date_format,                  0,     1 },
        { "time_format",              &cfg->time_format,                  0,     1 },
        { "snd_no_readings",          &cfg->snd_no_readings,              0,  1440 },
        { "snooze_timeout",           &cfg->snooze_timeout,               1,  1440 },
        { "alarm_repeat",             &cfg->alarm_repeat,                 0,  1440 },
        { "warning_volume",           &cfg->warning_volume,               0,   100 },
        { "alarm_volume",             &cfg->alarm_volume,                 0,   100 },
        { "brightness1",              &cfg->brightness1,                  0,   100 },
        { "brightness2",              &cfg->brightness2,                  0,   100 },
        { "brightness3",              &cfg->brightness3,                  0,   100 },
        { "restart_at_logged_errors", &cfg->restart_at_logged_errors,     0,  1000 },
        { "snd_loop_error",           &cfg->snd_loop_error,               0,     1 },
    };
    for (unsigned i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        if (strcmp(key, fields[i].name) == 0) {
            if (!parseIntIn(val, fields[i].lo, fields[i].hi, fields[i].dst))
                configError(cfg, key);
            return true;
        }
    }
    return false;
}

static bool applyStringKey(ParsedConfig *cfg, const char *key, const char *val) {
    struct { const char *name; char *dst; size_t size; } fields[] = {
        { "nightscout",      cfg->url,             sizeof(cfg->url) },
        { "token",           cfg->token,           sizeof(cfg->token) },
        { "name",            cfg->userName,        sizeof(cfg->userName) },
        { "device_name",     cfg->deviceName,      sizeof(cfg->deviceName) },
        { "restart_at_time", cfg->restart_at_time, sizeof(cfg->restart_at_time) },
        { "tz",              cfg->tz,              sizeof(cfg->tz) },
        { "ota_password",    cfg->otaPassword,     sizeof(cfg->otaPassword) },
        { "web_user",        cfg->webUser,         sizeof(cfg->webUser) },
        { "web_pass",        cfg->webPass,         sizeof(cfg->webPass) },
    };
    for (unsigned i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        if (strcmp(key, fields[i].name) == 0) {
            copyValue(cfg, key, fields[i].dst, fields[i].size, val);
            return true;
        }
    }
    return false;
}

bool applyConfigKey(ParsedConfig *cfg, const char *key, const char *val,
                    int thresholds_mgdl) {
    if (applyStringKey(cfg, key, val)) return true;
    if (applyGlucoseKey(cfg, key, val, thresholds_mgdl)) return true;
    if (applyIntKey(cfg, key, val)) return true;
    return false;
}

static int wlanSlotFromKey(const char *key, bool *isPass) {
    int idx;
    if (sscanf(key, "wlan_ssid_%d", &idx) == 1)      *isPass = false;
    else if (sscanf(key, "wlan_pass_%d", &idx) == 1) *isPass = true;
    else return -1;
    idx--;
    return (idx >= 0 && idx < CFG_MAX_WLAN) ? idx : -1;
}

static bool isTzChar(char c) {
    return isalnum((unsigned char)c) || (c != '\0' && strchr("<>+-:,./", c) != NULL);
}

bool tzStringValid(const char *tz) {
    if (!tz)
        return false;
    if (tz[0] == '\0')
        return true;
    for (const char *c = tz; *c; c++)
        if (!isTzChar(*c))
            return false;

    const char *p = tz;
    if (*p == '<') {
        const char *close = strchr(p, '>');
        if (!close || close - p - 1 < 3)
            return false;
        p = close + 1;
    } else {
        int letters = 0;
        while (isalpha((unsigned char)*p)) {
            p++;
            letters++;
        }
        if (letters < 3)
            return false;
    }
    if (*p == '+' || *p == '-')
        p++;
    return isdigit((unsigned char)*p) != 0;
}

bool configIsSecretKey(const char *key) {
    bool isPass;
    if (wlanSlotFromKey(key, &isPass) >= 0)
        return isPass;
    return strcmp(key, "token") == 0 || strcmp(key, "ota_password") == 0 ||
           strcmp(key, "web_pass") == 0;
}

static void clearSecret(ParsedConfig *cfg, const char *key) {
    if (!configIsSecretKey(key))
        return;
    bool isPass;
    int slot = wlanSlotFromKey(key, &isPass);
    if (slot >= 0)
        cfg->wlanpass[slot][0] = '\0';
    else
        applyConfigKey(cfg, key, "", 0);
}

void applyConfigForm(ParsedConfig *cfg, const ConfigKV *kv, int count) {
    int inputUnit = cfg->show_mgdl;
    cfg->configErrors   = 0;
    cfg->firstBadKey[0] = '\0';
    for (int i = 0; i < count; i++) {
        if (!kv[i].key || !kv[i].val) continue;
        if (strncmp(kv[i].key, "clear_", 6) == 0) {
            if (strcmp(kv[i].val, "1") == 0)
                clearSecret(cfg, kv[i].key + 6);
            continue;
        }
        if (kv[i].val[0] == '\0' && configIsSecretKey(kv[i].key))
            continue;
        bool isPass;
        int slot = wlanSlotFromKey(kv[i].key, &isPass);
        if (slot >= 0) {
            char *dst = isPass ? cfg->wlanpass[slot] : cfg->wlanssid[slot];
            strlcpy(dst, kv[i].val, sizeof(cfg->wlanssid[0]));
            continue;
        }
        applyConfigKey(cfg, kv[i].key, kv[i].val, inputUnit);
    }
    validateConfig(cfg);
}

int parseConfigBuffer(const char *buf, size_t len, ParsedConfig *cfg) {
    int parsed = 0;
    int currentWlan = -1;

    const char *p = buf;
    const char *end = buf + len;
    char lineBuf[CFG_MAX_LINE];

    while (p < end) {
        const char *eol = p;
        while (eol < end && *eol != '\n' && *eol != '\r')
            eol++;

        size_t lineLen = (size_t)(eol - p);
        bool truncated = lineLen >= sizeof(lineBuf);
        if (truncated)
            lineLen = sizeof(lineBuf) - 1;
        memcpy(lineBuf, p, lineLen);
        lineBuf[lineLen] = '\0';

        p = (eol < end) ? eol + 1 : end;

        char *line = trimWhitespace(lineBuf);

        if (line[0] == '\0' || line[0] == ';' || line[0] == '#')
            continue;

        if (line[0] == '[') {
            char *close = strchr(line, ']');
            if (close) {
                close[1] = '\0';
                *close = '\0';
                const char *section = line + 1;
                if (strcasecmp(section, "config") == 0) {
                    currentWlan = -1;
                } else if (strncasecmp(section, "wlan", 4) == 0) {
                    int idx = atoi(section + 4) - 1;
                    currentWlan = (idx >= 0 && idx < CFG_MAX_WLAN) ? idx : -2;
                } else {
                    currentWlan = -2;
                }
                *close = ']';
            } else {
                currentWlan = -2;
            }
            if (currentWlan == -2)
                configError(cfg, line);
            continue;
        }

        char *eq = strchr(line, '=');
        if (!eq) {
            if (truncated)
                configError(cfg, "long line");
            continue;
        }

        *eq = '\0';
        const char *key = trimWhitespace(line);
        const char *val = trimWhitespace(eq + 1);

        if (truncated)
            configError(cfg, key);

        if (currentWlan == -2)
            continue;

        if (currentWlan >= 0) {
            if (strcmp(key, "ssid") == 0) {
                copyValue(cfg, key, cfg->wlanssid[currentWlan], sizeof(cfg->wlanssid[currentWlan]), val);
                parsed++;
            } else if (strcmp(key, "pass") == 0) {
                copyValue(cfg, key, cfg->wlanpass[currentWlan], sizeof(cfg->wlanpass[currentWlan]), val);
                parsed++;
            } else {
                cfg->unknownKeys++;
                configError(cfg, key);
            }
        } else if (applyConfigKey(cfg, key, val, 0)) {
            parsed++;
        } else {
            cfg->unknownKeys++;
            configError(cfg, key);
        }
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

    if (!(cfg->red_low <= cfg->yellow_low && cfg->yellow_low < cfg->yellow_high &&
          cfg->yellow_high <= cfg->red_high)) {
        defaultBands(cfg);
        configError(cfg, "thresholds");
    }
    if (!(cfg->snd_alarm <= cfg->snd_warning && cfg->snd_warning < cfg->snd_warning_high &&
          cfg->snd_warning_high <= cfg->snd_alarm_high)) {
        defaultAlarmThresholds(cfg);
        configError(cfg, "thresholds");
    }

    if (!tzStringValid(cfg->tz)) {
        cfg->tz[0] = '\0';
        configError(cfg, "tz");
    }

    if (cfg->restart_at_time[0] == '\0') {
        strlcpy(cfg->restart_at_time, "NORES", sizeof(cfg->restart_at_time));
    } else if (strcmp(cfg->restart_at_time, "NORES") != 0 &&
               !restartTimeValid(cfg->restart_at_time)) {
        strlcpy(cfg->restart_at_time, "NORES", sizeof(cfg->restart_at_time));
        configError(cfg, "restart_at_time");
    }

    cfg->timeZone = clampInt(cfg->timeZone, -43200, 50400);
    cfg->dst      = clampInt(cfg->dst, 0, 7200);
    cfg->time_format = clampInt(cfg->time_format, 0, 1);
    cfg->restart_at_logged_errors = clampInt(cfg->restart_at_logged_errors, 0, 1000);

    cfg->show_mgdl         = clampInt(cfg->show_mgdl, 0, 1);
    cfg->show_current_time = clampInt(cfg->show_current_time, 0, 1);
    cfg->sgv_only          = clampInt(cfg->sgv_only, 0, 1);
    cfg->info_line         = clampInt(cfg->info_line, 0, 1);
    cfg->snd_loop_error    = clampInt(cfg->snd_loop_error, 0, 1);
    cfg->default_page      = clampInt(cfg->default_page, 0, 2);
    cfg->date_format       = clampInt(cfg->date_format, 0, 1);
    cfg->brightness1       = clampInt(cfg->brightness1, 0, 100);
    cfg->brightness2       = clampInt(cfg->brightness2, 0, 100);
    cfg->brightness3       = clampInt(cfg->brightness3, 0, 100);
    cfg->warning_volume    = clampInt(cfg->warning_volume, 0, 100);
    cfg->alarm_volume      = clampInt(cfg->alarm_volume, 0, 100);
    cfg->snooze_timeout    = clampInt(cfg->snooze_timeout, 1, 1440);
    cfg->alarm_repeat      = clampInt(cfg->alarm_repeat, 0, 1440);
    cfg->snd_no_readings   = clampInt(cfg->snd_no_readings, 0, 1440);
}

void formatConfigErrors(const ParsedConfig *cfg, char *out, size_t outSize) {
    if (cfg->configErrors == 0) {
        out[0] = '\0';
        return;
    }
    snprintf(out, outSize, "%d config error%s: %s", cfg->configErrors,
             cfg->configErrors == 1 ? "" : "s", cfg->firstBadKey);
}

bool configOtaEnabled(const ParsedConfig *cfg) {
    return cfg->otaPassword[0] != '\0';
}

bool configWebAuthEnabled(const ParsedConfig *cfg) {
    return cfg->webUser[0] != '\0' && cfg->webPass[0] != '\0';
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
    EMIT("tz = %s\n", cfg->tz);
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
    EMIT("ota_password = %s\n", cfg->otaPassword);
    EMIT("web_user = %s\n", cfg->webUser);
    EMIT("web_pass = %s\n", cfg->webPass);

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
