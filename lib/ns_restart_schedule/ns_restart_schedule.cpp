/*  ns_restart_schedule.cpp — Once-per-day scheduled restart
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ns_restart_schedule.h"

#include <stdio.h>
#include <string.h>

void restartScheduleInit(RestartSchedule *s) {
    s->armed = true;
}

bool restartTimeValid(const char *hhmm) {
    if (!hhmm)
        return false;
    int h, m;
    char extra;
    if (sscanf(hhmm, "%d:%d%c", &h, &m, &extra) != 2)
        return false;
    return h >= 0 && h <= 23 && m >= 0 && m <= 59;
}

bool restartScheduleDue(RestartSchedule *s, const char *hhmm,
                        int now_hour, int now_min) {
    if (!restartTimeValid(hhmm))
        return false;

    int h, m;
    sscanf(hhmm, "%d:%d", &h, &m);

    if (now_hour != h || now_min != m) {
        s->armed = true;
        return false;
    }

    if (!s->armed)
        return false;

    s->armed = false;
    return true;
}
