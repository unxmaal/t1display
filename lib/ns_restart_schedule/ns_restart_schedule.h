/*  ns_restart_schedule.h — Once-per-day scheduled restart
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NS_RESTART_SCHEDULE_H
#define NS_RESTART_SCHEDULE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct RestartSchedule {
    bool armed;
};

void restartScheduleInit(RestartSchedule *s);

/** True for a well-formed "HH:MM" (0-23, 0-59). "NORES" is not a time. */
bool restartTimeValid(const char *hhmm);

/**
 * True exactly once per entry into the configured minute.
 * "NORES", empty and malformed values never fire.
 */
bool restartScheduleDue(RestartSchedule *s, const char *hhmm,
                        int now_hour, int now_min);

#ifdef __cplusplus
}
#endif

#endif
