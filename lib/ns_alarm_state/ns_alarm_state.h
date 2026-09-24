/*  ns_alarm_state.h — Alarm repeat and snooze scheduling
 *
 *  Monotonic-time only. Nothing here needs a wall clock, so alarm
 *  behaviour does not depend on NTP having succeeded.
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NS_ALARM_STATE_H
#define NS_ALARM_STATE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ALARM_SNOOZE_MAX_MULT 3
#define ALARM_SNOOZE_MAX_SEC  (60UL * 60UL)

struct AlarmSchedule {
    unsigned long lastFiredMs;
    unsigned long snoozeStartMs;
    unsigned long snoozeDurationSec;
    int           snoozedLevel;
    int           snoozeMult;
    bool          hasFired;
};

void alarmScheduleInit(AlarmSchedule *s);

/** Severity ordering used to decide what a snooze may suppress. */
int alarmSeverityRank(int level);

/** Snooze the given level. Repeat presses stack up to ALARM_SNOOZE_MAX_MULT. */
void alarmScheduleSnooze(AlarmSchedule *s, unsigned long nowMs, int level,
                         int timeout_min);

/** Seconds left on the active snooze, or 0. */
unsigned long alarmSnoozeRemainingSec(const AlarmSchedule *s, unsigned long nowMs);

/** True when the level is due to sound: not normal, not snoozed, interval elapsed. */
bool alarmShouldFire(const AlarmSchedule *s, unsigned long nowMs, int level,
                     int alarm_repeat_min);

void alarmRecordFired(AlarmSchedule *s, unsigned long nowMs);

#ifdef __cplusplus
}
#endif

#endif
