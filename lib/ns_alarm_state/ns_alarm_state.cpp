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

#define DIRECTION_NONE 0
#define DIRECTION_LOW  1
#define DIRECTION_HIGH 2
#define DIRECTION_DATA 3

static int alarmDirection(int level) {
    switch (level) {
        case ALARM_LEVEL_LOW_ALARM:
        case ALARM_LEVEL_LOW_WARNING:
            return DIRECTION_LOW;
        case ALARM_LEVEL_HIGH_ALARM:
        case ALARM_LEVEL_HIGH_WARNING:
            return DIRECTION_HIGH;
        case ALARM_LEVEL_NO_READINGS:
        case ALARM_LEVEL_LOOP_ERROR:
            return DIRECTION_DATA;
        default:
            return DIRECTION_NONE;
    }
}

static uint32_t elapsedMs(uint32_t nowMs, uint32_t thenMs) {
    return nowMs - thenMs;
}

uint32_t alarmSnoozeRemainingSec(const AlarmSchedule *s, uint32_t nowMs) {
    if (s->snoozeDurationSec == 0)
        return 0;
    uint32_t goneSec = elapsedMs(nowMs, s->snoozeStartMs) / 1000U;
    if (goneSec >= s->snoozeDurationSec)
        return 0;
    return s->snoozeDurationSec - goneSec;
}

void alarmScheduleSnooze(AlarmSchedule *s, uint32_t nowMs, int level,
                         int timeout_min) {
    if (timeout_min < 1)
        timeout_min = 1;
    uint32_t step = (uint32_t)timeout_min * 60U;

    uint32_t dur = step;
    uint32_t remaining = alarmSnoozeRemainingSec(s, nowMs);
    if (remaining > 0 && level == s->snoozedLevel) {
        if (elapsedMs(nowMs, s->snoozeStartMs) < ALARM_SNOOZE_DEBOUNCE_MS)
            return;
        dur = remaining + step;
    }
    if (dur > ALARM_SNOOZE_MAX_SEC)
        dur = ALARM_SNOOZE_MAX_SEC;

    s->snoozeStartMs     = nowMs;
    s->snoozeDurationSec = dur;
    s->snoozedLevel      = level;
}

bool alarmShouldFire(const AlarmSchedule *s, uint32_t nowMs, int level,
                     int alarm_repeat_min) {
    if (level == ALARM_LEVEL_NORMAL)
        return false;

    if (alarmSnoozeRemainingSec(s, nowMs) > 0 &&
        alarmDirection(level) == alarmDirection(s->snoozedLevel) &&
        alarmSeverityRank(level) <= alarmSeverityRank(s->snoozedLevel))
        return false;

    if (!s->hasFired)
        return true;

    uint32_t repeatMs = (uint32_t)alarm_repeat_min * 60U * 1000U;
    return elapsedMs(nowMs, s->lastFiredMs) > repeatMs;
}

void alarmRecordFired(AlarmSchedule *s, uint32_t nowMs) {
    s->lastFiredMs = nowMs;
    s->hasFired    = true;
}

int alarmSound(int level) {
    switch (level) {
        case ALARM_LEVEL_LOW_ALARM:    return ALARM_SOUND_LOW_ALARM;
        case ALARM_LEVEL_HIGH_ALARM:   return ALARM_SOUND_HIGH_ALARM;
        case ALARM_LEVEL_LOW_WARNING:  return ALARM_SOUND_LOW_WARNING;
        case ALARM_LEVEL_HIGH_WARNING: return ALARM_SOUND_HIGH_WARNING;
        case ALARM_LEVEL_NO_READINGS:
        case ALARM_LEVEL_LOOP_ERROR:   return ALARM_SOUND_NO_READINGS;
        default:                       return ALARM_SOUND_NONE;
    }
}
