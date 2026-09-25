/*  ns_json_parse.cpp — Nightscout JSON response parsing
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ns_json_parse.h"
#include "ns_pure_logic.h"

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>
#include <string.h>
#include <stdio.h>

/* ── Nightscout SGV response ───────────────────────────────────── */

int parseSGVResponse(const char *json, size_t len, SGVEntry *entry) {
    if (!json || !entry)
        return PARSE_ERR_JSON;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json, len);
    if (err)
        return PARSE_ERR_JSON;

    JsonArray arr = doc.as<JsonArray>();
    if (arr.isNull() || arr.size() == 0)
        return PARSE_ERR_EMPTY;

    // Find first entry with "sgv" field
    int sgvIdx = -1;
    for (int i = 0; i < (int)arr.size(); i++) {
        if (arr[i]["sgv"].is<float>() || arr[i]["sgv"].is<int>()) {
            sgvIdx = i;
            break;
        }
    }
    if (sgvIdx < 0)
        return PARSE_ERR_NO_SGV;

    JsonObject obj = arr[sgvIdx];

    strlcpy(entry->device, obj["device"] | "N/A", sizeof(entry->device));
    entry->date_ms  = static_cast<uint64_t>(obj["date"].as<long long>());
    entry->date_sec = entry->date_ms / 1000;

    // Direction: try "direction", fall back to numeric "trend"
    const char *dir = obj["direction"] | "";
    if (dir[0] == '\0' || strcmp(dir, "N/A") == 0) {
        // Some sources (Railway, xDrip) use numeric "trend"
        if (obj["trend"].is<int>()) {
            int trend = obj["trend"].as<int>();
            switch (trend) {
                case 1: dir = "DoubleUp"; break;
                case 2: dir = "SingleUp"; break;
                case 3: dir = "FortyFiveUp"; break;
                case 4: dir = "Flat"; break;
                case 5: dir = "FortyFiveDown"; break;
                case 6: dir = "SingleDown"; break;
                case 7: dir = "DoubleDown"; break;
                default: dir = "NONE"; break;
            }
        } else {
            // Try string "trend" (another variant)
            const char *trendStr = obj["trend"] | "";
            if (trendStr[0] != '\0')
                dir = trendStr;
            else
                dir = "NONE";
        }
    }
    strlcpy(entry->direction, dir, sizeof(entry->direction));

    entry->sgv_mgdl = obj["sgv"].as<float>();
    entry->sgv_mmol = entry->sgv_mgdl / MGDL_PER_MMOL;
    entry->arrow_angle = directionToAngle(entry->direction);

    return PARSE_OK;
}

/* ── Delta/properties response ─────────────────────────────────── */

int parseDeltaResponse(const char *json, size_t len, DeltaInfo *delta) {
    if (!json || !delta)
        return PARSE_ERR_JSON;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json, len);
    if (err)
        return PARSE_ERR_JSON;

    if (!doc["delta"]["mgdl"].is<int>())
        return PARSE_ERR_NO_SGV;

    delta->mgdl = doc["delta"]["mgdl"].as<int>();
    delta->mmol = delta->mgdl / MGDL_PER_MMOL;

    return PARSE_OK;
}

/* ── Delta formatting ──────────────────────────────────────────── */

void formatDelta(char *buf, size_t bufsize, const DeltaInfo *delta, bool show_mgdl) {
    if (show_mgdl)
        snprintf(buf, bufsize, "%+d", delta->mgdl);
    else
        snprintf(buf, bufsize, "%+.1f", delta->mmol);
}
