/*  ns_json_parse.h — Nightscout JSON response parsing
 *
 *  Pure parsing logic: takes JSON strings, populates structs.
 *  No HTTP, no WiFi, no hardware. Compiles on host for testing.
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NS_JSON_PARSE_H
#define NS_JSON_PARSE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>

/* ── Parsed glucose entry ──────────────────────────────────────── */

struct SGVEntry {
    char     device[64];
    uint64_t date_ms;       // milliseconds since epoch
    time_t   date_sec;      // seconds since epoch
    char     direction[32]; // "Flat", "SingleUp", etc.
    float    sgv_mgdl;      // raw mg/dL value
    float    sgv_mmol;      // converted to mmol/L
    int      arrow_angle;   // from directionToAngle()
};

/* ── Parsed delta ──────────────────────────────────────────────── */

struct DeltaInfo {
    int   mgdl;
    float mmol;
};

/* ── Parse results ─────────────────────────────────────────────── */

#define PARSE_OK              0
#define PARSE_ERR_JSON       -1   // JSON deserialization failed
#define PARSE_ERR_EMPTY      -2   // Valid JSON but no SGV entries
#define PARSE_ERR_NO_SGV     -3   // Array exists but no entry has "sgv"

/**
 * Parse a Nightscout /api/v1/entries.json response.
 * Populates entry with the first SGV-containing object.
 * Returns PARSE_OK on success, PARSE_ERR_* on failure.
 */
int parseSGVResponse(const char *json, size_t len, SGVEntry *entry);

/**
 * Parse a Nightscout /api/v2/properties/delta response.
 * Returns PARSE_ERR_NO_SGV when the response carries no delta value, so an
 * absent delta is never reported as a delta of zero.
 */
int parseDeltaResponse(const char *json, size_t len, DeltaInfo *delta);

/**
 * Format a delta value into a display string.
 *   show_mgdl=true  → "+5" or "-3"
 *   show_mgdl=false → "+0.3" or "-0.2"
 */
void formatDelta(char *buf, size_t bufsize, const DeltaInfo *delta, bool show_mgdl);

#endif // NS_JSON_PARSE_H
