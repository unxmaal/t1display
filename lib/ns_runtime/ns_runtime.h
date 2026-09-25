/*  ns_runtime.h — Firmware timing budget and service start latch
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NS_RUNTIME_H
#define NS_RUNTIME_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NS_WDT_TIMEOUT_SEC          30
#define NS_HTTP_CONNECT_TIMEOUT_MS  5000
#define NS_HTTP_READ_TIMEOUT_MS     8000
#define NS_FETCHES_PER_WDT_RESET    1
#define NS_FETCH_WINDOW_MS \
    ((unsigned long)NS_FETCHES_PER_WDT_RESET * (NS_HTTP_CONNECT_TIMEOUT_MS + NS_HTTP_READ_TIMEOUT_MS))

struct ServicesLatch {
    bool started;
};

void servicesLatchInit(ServicesLatch *l);

/** True exactly once: the first time Wi-Fi is observed connected. */
bool servicesDue(ServicesLatch *l, bool wifiConnected);

#ifdef __cplusplus
}
#endif

#endif
