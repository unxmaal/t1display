/*  ns_url_build.cpp — Nightscout request URL assembly
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ns_url_build.h"

#include <string.h>

#ifndef strlcat
extern "C" size_t strlcat(char *dst, const char *src, size_t dsize);
#endif

void nsBuildBaseUrl(char *out, size_t outSize, const char *url) {
    out[0] = '\0';
    if (url[0] == '\0')
        return;
    if (strncmp(url, "http", 4) != 0)
        strlcat(out, "https://", outSize);
    strlcat(out, url, outSize);

    size_t len = strlen(out);
    if (len > 0 && out[len - 1] == '/')
        out[len - 1] = '\0';
}

void nsAppendEntriesPath(char *url, size_t urlSize) {
    strlcat(url, "/api/v1/entries.json?count=1&find[type][$eq]=sgv", urlSize);
}

void nsAppendPropertiesPath(char *url, size_t urlSize) {
    strlcat(url, "/api/v2/properties/delta", urlSize);
}

void nsAppendToken(char *url, size_t urlSize, const char *token) {
    if (token[0] == '\0')
        return;
    strlcat(url, strchr(url, '?') ? "&token=" : "?token=", urlSize);
    strlcat(url, token, urlSize);
}
