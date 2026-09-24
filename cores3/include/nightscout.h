/*  nightscout.h
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CORES3_NIGHTSCOUT_H
#define CORES3_NIGHTSCOUT_H

#include "config.h"

/**
 * Poll Nightscout API and populate NSinfo struct.
 * Makes up to 2 HTTP requests:
 *   1. /api/v1/entries.json?count=1 — newest SGV entry
 *   2. /api/v2/properties/delta — delta
 *
 * Returns 0 on success, error code on failure.
 * Errors are also logged to errLog.
 */
int readNightscout(const Config &cfg, NSinfo &ns, ErrorLog &errLog);

#endif // CORES3_NIGHTSCOUT_H
