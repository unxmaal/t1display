/*  ns_error_log.h — Bounded fetch-error history
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NS_ERROR_LOG_H
#define NS_ERROR_LOG_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NS_ERR_LOG_SIZE 10

struct NsErrorLog {
    int           codes[NS_ERR_LOG_SIZE];
    long          times[NS_ERR_LOG_SIZE];
    int           held;
    int           next;
    int           consecutive;
    unsigned long total;
};

void nsErrorLogInit(NsErrorLog *l);

/** Record a failure. when_sec may be 0 when no clock is available. */
void nsErrorLogAdd(NsErrorLog *l, int code, long when_sec);

/** Record a successful fetch. Clears the consecutive-failure run. */
void nsErrorLogSuccess(NsErrorLog *l);

int  nsErrorLogHeld(const NsErrorLog *l);
int  nsErrorLogCodeAt(const NsErrorLog *l, int i);
long nsErrorLogTimeAt(const NsErrorLog *l, int i);

/** True while the last fetch attempt failed. */
bool nsErrorLogHasActiveFault(const NsErrorLog *l);

/** Restart only on a sustained run of failures, never on a cumulative total. */
bool nsErrorLogShouldRestart(const NsErrorLog *l, int threshold);

#ifdef __cplusplus
}
#endif

#endif
