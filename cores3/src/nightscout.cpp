/*  nightscout.cpp
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "nightscout.h"
#include "ns_pure_logic.h"
#include "ns_json_parse.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <string.h>
#include <stdio.h>

/* Encrypted but no cert validation — common for self-signed Nightscout
   instances. Swap setInsecure() for setCACert() to pin a certificate. */
static WiFiClientSecure secureClient;
static bool secureClientInit = false;

#define NS_LOG Serial

/* ── ErrorLog ──────────────────────────────────────────────────── */

static long nowEpochOrZero() {
    struct tm t;
    return getLocalTime(&t, 10) ? (long)mktime(&t) : 0;
}

static void logError(ErrorLog &errLog, int code) {
    nsErrorLogAdd(&errLog, code, nowEpochOrZero());
}

/* ── URL builder ───────────────────────────────────────────────── */

static void buildBaseUrl(char *out, size_t outSize, const char *url) {
    out[0] = '\0';
    if (strncmp(url, "http", 4) != 0)
        strlcat(out, "https://", outSize);
    strlcat(out, url, outSize);

    // strip trailing slash
    size_t len = strlen(out);
    if (len > 0 && out[len - 1] == '/')
        out[len - 1] = '\0';
}

static void appendToken(char *url, size_t urlSize, const char *token) {
    if (token[0] == '\0')
        return;
    if (strchr(url, '?'))
        strlcat(url, "&token=", urlSize);
    else
        strlcat(url, "?token=", urlSize);
    strlcat(url, token, urlSize);
}

/* ── HTTP fetch + sanitize helper ─────────────────────────────── */

static int httpGetSanitized(const char *url, char **outBuf, size_t *outLen,
                             ErrorLog &errLog, int errCode) {
    HTTPClient http;
    http.setConnectTimeout(10000);
    http.setTimeout(15000);
    NS_LOG.printf("[HTTP] GET %s\n", url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    if (strncmp(url, "https", 5) == 0) {
        if (!secureClientInit) {
            secureClient.setInsecure();
            secureClientInit = true;
        }
        http.begin(secureClient, url);
    } else {
        http.begin(url);
    }
    int httpCode = http.GET();
    NS_LOG.printf("[HTTP] Response: %d\n", httpCode);
    if (httpCode <= 0) {
        logError(errLog, httpCode);
        http.end();
        return httpCode;
    }
    if (httpCode != 200) {
        logError(errLog, httpCode);
        http.end();
        return httpCode;
    }

    String json = http.getString();
    http.end();
    NS_LOG.printf("[HTTP] Body: %d bytes\n", (int)json.length());

    // Sanitize into a mutable buffer
    size_t jsonLen = json.length();
    char *buf = new char[jsonLen + 1];
    json.toCharArray(buf, jsonLen + 1);
    jsonLen = sanitizeJson(buf, jsonLen);

    *outBuf = buf;
    *outLen = jsonLen;
    return 0;
}

/* ── First request: SGV data ───────────────────────────────────── */

static int fetchSGV(const Config &cfg, NSinfo &ns, ErrorLog &errLog) {
    char nsUrl[320];
    buildBaseUrl(nsUrl, sizeof(nsUrl), cfg.url);

    bool isSugarmate = (strstr(nsUrl, "sugarmate") != nullptr);

    if (!isSugarmate) {
        strlcat(nsUrl, "/api/v1/entries.json?count=10", sizeof(nsUrl));
        if (cfg.sgv_only)
            strlcat(nsUrl, "&find[type][$eq]=sgv", sizeof(nsUrl));
    }
    appendToken(nsUrl, sizeof(nsUrl), cfg.token);

    char *buf = nullptr;
    size_t bufLen = 0;
    int rc = httpGetSanitized(nsUrl, &buf, &bufLen, errLog, ERR_JSON_PARSE);
    if (rc != 0)
        return rc;

    SGVEntry entry;
    DeltaInfo delta;
    int parseResult;

    if (isSugarmate) {
        parseResult = parseSugarmateResponse(buf, bufLen, &entry, &delta);
    } else {
        parseResult = parseSGVResponse(buf, bufLen, &entry);
    }
    delete[] buf;

    if (parseResult != PARSE_OK) {
        int code = (parseResult == PARSE_ERR_EMPTY) ? ERR_NO_DATA : ERR_JSON_PARSE;
        logError(errLog, code);
        return code;
    }

    // Map SGVEntry → NSinfo
    strlcpy(ns.sensDev, entry.device, sizeof(ns.sensDev));
    ns.rawtime = entry.date_ms;
    ns.sensTime = entry.date_sec;
    localtime_r(&ns.sensTime, &ns.sensTm);
    strlcpy(ns.sensDir, entry.direction, sizeof(ns.sensDir));
    ns.sensSgvMgDl = entry.sgv_mgdl;
    ns.sensSgv = entry.sgv_mmol;
    ns.arrowAngle = entry.arrow_angle;

    // Sugarmate provides delta in the SGV response
    if (isSugarmate) {
        ns.delta_mgdl = delta.mgdl;
        ns.delta_scaled = delta.mmol;
        formatDelta(ns.delta_display, sizeof(ns.delta_display),
                    &delta, cfg.show_mgdl);
    }

    return 0;
}

/* ── Second request: delta/properties ──────────────────────────── */

static int fetchProperties(const Config &cfg, NSinfo &ns, ErrorLog &errLog) {
    char nsUrl[320];
    buildBaseUrl(nsUrl, sizeof(nsUrl), cfg.url);
    strlcat(nsUrl, "/api/v2/properties/delta", sizeof(nsUrl));
    appendToken(nsUrl, sizeof(nsUrl), cfg.token);

    char *buf = nullptr;
    size_t bufLen = 0;
    int rc = httpGetSanitized(nsUrl, &buf, &bufLen, errLog, ERR_JSON2_PARSE);
    if (rc != 0)
        return rc;

    DeltaInfo delta;
    int parseResult = parseDeltaResponse(buf, bufLen, &delta);
    delete[] buf;

    if (parseResult != PARSE_OK) {
        logError(errLog, ERR_JSON2_PARSE);
        return ERR_JSON2_PARSE;
    }

    ns.delta_mgdl = delta.mgdl;
    ns.delta_scaled = delta.mmol;
    formatDelta(ns.delta_display, sizeof(ns.delta_display),
                &delta, cfg.show_mgdl);

    return 0;
}

/* ── Public API ────────────────────────────────────────────────── */

int readNightscout(const Config &cfg, NSinfo &ns, ErrorLog &errLog) {
    if (cfg.url[0] == '\0') {
        logError(errLog, ERR_NO_DATA);
        return ERR_NO_DATA;
    }

    int rc = fetchSGV(cfg, ns, errLog);
    if (rc != 0)
        return rc;

    // Sugarmate already provides delta in the SGV response
    if (strstr(cfg.url, "sugarmate") == nullptr)
        fetchProperties(cfg, ns, errLog);

    nsErrorLogSuccess(&errLog);
    return 0;
}
