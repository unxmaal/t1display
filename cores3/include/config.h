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
#include "ns_shared.h"

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

/* ── State shared with the web task ───────────────────────────── */

struct SaveResult {
    int  wrote;
    bool sdOk;
    char errors[48];
};

struct PowerStatus {
    int  pct;
    int  mv;
    bool charging;
};

struct WebShared {
    Guarded<Config>              *cfg;
    Exchange<Config, SaveResult> *save;
    Guarded<PowerStatus>         *power;
};

/* ── Error log ─────────────────────────────────────────────────── */

#include "ns_error_log.h"

typedef NsErrorLog ErrorLog;

/* ── Error codes ───────────────────────────────────────────────── */

#define ERR_JSON_PARSE     1001
#define ERR_NO_DATA        1002
#define ERR_JSON2_PARSE    1003

#endif // CORES3_CONFIG_H
