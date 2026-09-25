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
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ALARM_SNOOZE_MAX_SEC     (60U * 60U)
#define ALARM_SNOOZE_DEBOUNCE_MS 10000U

struct AlarmSchedule {
    uint32_t lastFiredMs;
    uint32_t snoozeStartMs;
    uint32_t snoozeDurationSec;
    int           snoozedLevel;
    bool          hasFired;
};

void alarmScheduleInit(AlarmSchedule *s);

/** Severity ordering used to decide what a snooze may suppress. */
int alarmSeverityRank(int level);

/**
 * Snooze the given level for timeout_min (at least 1). A press for the same
 * level within ALARM_SNOOZE_DEBOUNCE_MS of the last is ignored; a later one
 * adds another timeout_min, capped at ALARM_SNOOZE_MAX_SEC.
 */
void alarmScheduleSnooze(AlarmSchedule *s, uint32_t nowMs, int level,
                         int timeout_min);

/** Seconds left on the active snooze, or 0. */
uint32_t alarmSnoozeRemainingSec(const AlarmSchedule *s, uint32_t nowMs);

/**
 * True when the level is due to sound: not normal, interval elapsed, and not
 * covered by the snooze. A snooze covers only levels in the same direction
 * (low, high, or data) that are no more severe than the snoozed one.
 */
bool alarmShouldFire(const AlarmSchedule *s, uint32_t nowMs, int level,
                     int alarm_repeat_min);

void alarmRecordFired(AlarmSchedule *s, uint32_t nowMs);

#define ALARM_SOUND_NONE         0
#define ALARM_SOUND_LOW_ALARM    1
#define ALARM_SOUND_HIGH_ALARM   2
#define ALARM_SOUND_LOW_WARNING  3
#define ALARM_SOUND_HIGH_WARNING 4
#define ALARM_SOUND_NO_READINGS  5

int alarmSound(int level);

/** True when a sound plays at alarm_volume rather than warning_volume. */
bool alarmSoundUsesAlarmVolume(int sound);

#ifdef __cplusplus
}
#endif

#endif
