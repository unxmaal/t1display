/*  ns_url_build.h — Nightscout request URL assembly
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NS_URL_BUILD_H
#define NS_URL_BUILD_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Normalize a configured URL: add https:// if no scheme, strip trailing slash. */
void nsBuildBaseUrl(char *out, size_t outSize, const char *url);

/** Append the entries endpoint for the single newest sgv record. */
void nsAppendEntriesPath(char *url, size_t urlSize);

/** Append the v2 delta properties endpoint. */
void nsAppendPropertiesPath(char *url, size_t urlSize);

/** Append the API token, choosing ? or & as appropriate. */
void nsAppendToken(char *url, size_t urlSize, const char *token);

#ifdef __cplusplus
}
#endif

#endif
