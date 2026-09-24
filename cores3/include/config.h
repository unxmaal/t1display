/*  config.h
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CORES3_CONFIG_H
#define CORES3_CONFIG_H

#include <stdint.h>
#include <time.h>
#include "ns_config_parse.h"

/* ── Device config (loaded from SD card INI) ───────────────────── */

// Config is ParsedConfig — parsed by ns_config_parse lib.
// Use configDefaults() to initialize, parseConfigBuffer() to load from INI.
typedef ParsedConfig Config;

/* ── Nightscout data (populated from API responses) ────────────── */

struct NSinfo {
    char     sensDev[64];
    uint64_t rawtime       = 0;
    time_t   sensTime      = 0;
    struct tm sensTm;
    char     sensDir[32];
    float    sensSgvMgDl   = 0;
    float    sensSgv       = 0;       // mmol/L
    int      arrowAngle    = 180;

    /* Delta */
    int      delta_mgdl    = 0;
    float    delta_scaled  = 0;       // mmol/L
    char     delta_display[16];
};

/* ── Error log ─────────────────────────────────────────────────── */

#define ERR_LOG_SIZE 10

struct ErrorLogEntry {
    struct tm err_time;
    int       err_code;
};

struct ErrorLog {
    ErrorLogEntry entries[ERR_LOG_SIZE];
    int ptr   = 0;       // next insertion index (0–9)
    int count = 0;       // total errors since boot

    void add(int code);
};

/* ── Error codes ───────────────────────────────────────────────── */

#define ERR_JSON_PARSE     1001
#define ERR_NO_DATA        1002
#define ERR_JSON2_PARSE    1003

#endif // CORES3_CONFIG_H
