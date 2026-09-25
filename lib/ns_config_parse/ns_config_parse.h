/*  ns_config_parse.h — INI config file parsing
 *
 *  Parses M5NS.INI format from a char buffer (no SD/file deps).
 *  The caller reads the file into memory; this lib parses it.
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NS_CONFIG_PARSE_H
#define NS_CONFIG_PARSE_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Parsed config values ──────────────────────────────────────── */

#define CFG_MAX_WLAN 10

#define CFG_MAX_LINE 256
#define CFG_GLUCOSE_MIN 1.0f
#define CFG_GLUCOSE_MAX 40.0f

struct ParsedConfig {
    char url[128];
    char token[64];
    char userName[32];
    char deviceName[32];

    int timeZone;
    int dst;

    int show_mgdl;
    int show_current_time;
    int default_page;
    int sgv_only;
    int info_line;
    int date_format;
    int time_format;

    float yellow_low;
    float yellow_high;
    float red_low;
    float red_high;

    float snd_alarm;
    float snd_warning;
    float snd_alarm_high;
    float snd_warning_high;
    int   snd_no_readings;

    int snooze_timeout;
    int alarm_repeat;
    int warning_volume;
    int alarm_volume;

    int brightness1;
    int brightness2;
    int brightness3;

    int restart_at_logged_errors;
    char restart_at_time[10];

    char otaPassword[64];
    char webUser[32];
    char webPass[64];

    int snd_loop_error;

    int unknownKeys;
    int configErrors;
    char firstBadKey[24];

    char wlanssid[CFG_MAX_WLAN][64];
    char wlanpass[CFG_MAX_WLAN][64];
};

struct ConfigKV {
    const char *key;
    const char *val;
};

/**
 * Apply one key/value pair. thresholds_mgdl states the unit the incoming
 * threshold values are expressed in. Returns true if the key was recognized.
 */
bool applyConfigKey(ParsedConfig *cfg, const char *key, const char *val,
                    int thresholds_mgdl);

/**
 * Apply a whole form submission. Threshold values are interpreted in the
 * unit the config was in before the call, so a show_mgdl change in the same
 * submission does not rescale them. Validates when done. Config errors are
 * reset first, so they describe this submission alone.
 */
void applyConfigForm(ParsedConfig *cfg, const ConfigKV *kv, int count);

/**
 * Initialize a ParsedConfig with default values.
 */
void configDefaults(ParsedConfig *cfg);

/**
 * Parse an INI buffer into a ParsedConfig.
 * Reads exactly len bytes and does not require a terminator at buf[len].
 * Lines longer than CFG_MAX_LINE are truncated.
 * Returns number of config values successfully parsed.
 */
int parseConfigBuffer(const char *buf, size_t len, ParsedConfig *cfg);

/**
 * Validate and clamp all integer fields to safe ranges.
 */
void validateConfig(ParsedConfig *cfg);

/** OTA is only started when a password is configured. */
/** "" when clean, else "N config error(s): <first bad key>". */
void formatConfigErrors(const ParsedConfig *cfg, char *out, size_t outSize);

bool configOtaEnabled(const ParsedConfig *cfg);

/** Web config requires auth only when both user and password are set. */
bool configWebAuthEnabled(const ParsedConfig *cfg);

/**
 * Serialize a ParsedConfig to INI text (inverse of parseConfigBuffer).
 * Writes into buf up to bufSize bytes (including null terminator).
 * Returns bytes written (excluding null terminator), or 0 if buffer too small.
 */
int serializeConfigINI(const ParsedConfig *cfg, char *buf, size_t bufSize);

#ifdef __cplusplus
}
#endif

#endif // NS_CONFIG_PARSE_H
