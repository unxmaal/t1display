/*  ns_error_log.cpp — Bounded fetch-error history
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ns_error_log.h"

#include <string.h>

void nsErrorLogInit(NsErrorLog *l) {
    memset(l, 0, sizeof(*l));
}

void nsErrorLogAdd(NsErrorLog *l, int code, long when_sec) {
    if (l->held < NS_ERR_LOG_SIZE) {
        l->codes[l->held] = code;
        l->times[l->held] = when_sec;
        l->held++;
    } else {
        for (int i = 0; i < NS_ERR_LOG_SIZE - 1; i++) {
            l->codes[i] = l->codes[i + 1];
            l->times[i] = l->times[i + 1];
        }
        l->codes[NS_ERR_LOG_SIZE - 1] = code;
        l->times[NS_ERR_LOG_SIZE - 1] = when_sec;
    }
    l->consecutive++;
    l->total++;
}

void nsErrorLogSuccess(NsErrorLog *l) {
    l->consecutive = 0;
}

int nsErrorLogHeld(const NsErrorLog *l) {
    return l->held;
}

int nsErrorLogCodeAt(const NsErrorLog *l, int i) {
    if (i < 0 || i >= l->held) return 0;
    return l->codes[i];
}

long nsErrorLogTimeAt(const NsErrorLog *l, int i) {
    if (i < 0 || i >= l->held) return 0;
    return l->times[i];
}

bool nsErrorLogHasActiveFault(const NsErrorLog *l) {
    return l->consecutive > 0;
}

bool nsErrorLogShouldRestart(const NsErrorLog *l, int threshold) {
    if (threshold <= 0) return false;
    return l->consecutive >= threshold;
}
