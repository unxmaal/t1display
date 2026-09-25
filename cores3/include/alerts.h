/*  alerts.h
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CORES3_ALERTS_H
#define CORES3_ALERTS_H

#include "config.h"
#include "ns_alarm_state.h"

struct AlarmState {
    AlarmSchedule sched;

    void snooze(uint32_t nowMs, int level, int timeout_min);
    uint32_t snoozeRemaining(uint32_t nowMs) const;
};

/**
 * Check alarm conditions and play sound if needed.
 * Call every loop iteration after Nightscout data is updated.
 */
void checkAlarms(const Config &cfg, const NSinfo &ns, AlarmState &alarm);

/** Alarm level currently indicated by the data, without side effects. */
int currentAlarmLevel(const Config &cfg, const NSinfo &ns);

/** Advance melody playback. Call every loop iteration. */
void serviceAlerts();

/** Individual alert melodies — for testing via web UI. */
void playLowAlarm(int volume);
void playLowWarning(int volume);
void playHighAlarm(int volume);
void playHighWarning(int volume);
void playNoReadings(int volume);

#endif // CORES3_ALERTS_H
