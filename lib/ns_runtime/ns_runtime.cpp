/*  ns_runtime.cpp — Firmware timing budget and service start latch
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ns_runtime.h"

void servicesLatchInit(ServicesLatch *l) {
    l->started = false;
}

bool servicesDue(ServicesLatch *l, bool wifiConnected) {
    if (l->started || !wifiConnected)
        return false;
    l->started = true;
    return true;
}
