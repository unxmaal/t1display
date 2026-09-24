/*  ns_alarm_state.cpp — Alarm repeat and snooze scheduling
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ns_alarm_state.h"
#include "ns_pure_logic.h"

#include <string.h>

void alarmScheduleInit(AlarmSchedule *s) {
    memset(s, 0, sizeof(*s));
    s->snoozedLevel = ALARM_LEVEL_NORMAL;
}

int alarmSeverityRank(int level) {
    switch (level) {
        case ALARM_LEVEL_LOW_ALARM:
        case ALARM_LEVEL_HIGH_ALARM:
            return 3;
        case ALARM_LEVEL_LOOP_ERROR:
        case ALARM_LEVEL_LOW_WARNING:
        case ALARM_LEVEL_HIGH_WARNING:
            return 2;
        case ALARM_LEVEL_NO_READINGS:
            return 1;
        default:
            return 0;
    }
}

static unsigned long elapsedMs(unsigned long nowMs, unsigned long thenMs) {
    return nowMs - thenMs;
}

unsigned long alarmSnoozeRemainingSec(const AlarmSchedule *s, unsigned long nowMs) {
    if (s->snoozeDurationSec == 0)
        return 0;
    unsigned long goneSec = elapsedMs(nowMs, s->snoozeStartMs) / 1000UL;
    if (goneSec >= s->snoozeDurationSec)
        return 0;
    return s->snoozeDurationSec - goneSec;
}

void alarmScheduleSnooze(AlarmSchedule *s, unsigned long nowMs, int level,
                         int timeout_min) {
    if (timeout_min <= 0)
        return;

    if (alarmSnoozeRemainingSec(s, nowMs) == 0)
        s->snoozeMult = 0;

    if (s->snoozeMult < ALARM_SNOOZE_MAX_MULT)
        s->snoozeMult++;

    unsigned long dur = (unsigned long)timeout_min * 60UL * (unsigned long)s->snoozeMult;
    if (dur > ALARM_SNOOZE_MAX_SEC)
        dur = ALARM_SNOOZE_MAX_SEC;

    s->snoozeStartMs     = nowMs;
    s->snoozeDurationSec = dur;
    s->snoozedLevel      = level;
}

bool alarmShouldFire(const AlarmSchedule *s, unsigned long nowMs, int level,
                     int alarm_repeat_min) {
    if (level == ALARM_LEVEL_NORMAL)
        return false;

    if (alarmSnoozeRemainingSec(s, nowMs) > 0 &&
        alarmSeverityRank(level) <= alarmSeverityRank(s->snoozedLevel))
        return false;

    if (!s->hasFired)
        return true;

    unsigned long repeatMs = (unsigned long)alarm_repeat_min * 60UL * 1000UL;
    return elapsedMs(nowMs, s->lastFiredMs) > repeatMs;
}

void alarmRecordFired(AlarmSchedule *s, unsigned long nowMs) {
    s->lastFiredMs = nowMs;
    s->hasFired    = true;
}
