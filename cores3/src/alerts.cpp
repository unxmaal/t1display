/*  alerts.cpp
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "alerts.h"
#include "ns_pure_logic.h"

#include <M5Unified.h>
#include <time.h>

/* ── AlarmState ────────────────────────────────────────────────── */

void AlarmState::snooze(int timeout_min) {
    snoozeMult++;
    struct tm now;
    if (getLocalTime(&now, 10)) {
        snoozeUntil = mktime(&now) + (timeout_min * snoozeMult * 60);
    }
}

int AlarmState::snoozeRemaining() const {
    struct tm now;
    if (!getLocalTime(&now, 10))
        return 0;
    int rem = (int)difftime(snoozeUntil, mktime(&now));
    return (rem > 0) ? rem : 0;
}

bool AlarmState::shouldFire(int alarm_repeat_min) const {
    struct tm now;
    if (!getLocalTime(&now, 10))
        return false;
    int elapsed = (int)difftime(mktime(&now), lastAlarmTime);
    return elapsed > (alarm_repeat_min * 60);
}

void AlarmState::recordFired() {
    struct tm now;
    if (getLocalTime(&now, 10))
        lastAlarmTime = mktime(&now);
}

/* ── Sound helpers ─────────────────────────────────────────────── */

// Scale config volume (0-100) to speaker range (0-255)
static int scaleVolume(int vol) {
    return (vol * 255) / 100;
}

static void playMelody(int volume, const int *notes, const int *durations, int count) {
    M5.Speaker.setVolume(scaleVolume(volume));
    for (int i = 0; i < count; i++) {
        M5.Speaker.tone(notes[i], durations[i]);
        delay(durations[i] + 60);
    }
}

// Low alarm: descending tritone + chromatic fall — psychoacoustically urgent
// B5→F5 (tritone), then chromatic descent E5→Eb5→D5→Db5 (falling sensation)
// Repeated twice with shorter gaps for urgency
void playLowAlarm(int volume) {
    const int notes[]    = { 988, 698,  659, 622, 587, 554,
                             988, 698,  659, 622, 587, 554 };
    const int durations[] = { 120, 200,  100, 100, 100, 250,
                              120, 200,  100, 100, 100, 250 };
    playMelody(volume, notes, durations, 12);
}

// Low warning: gentle descending three-note — B5 G5 D5
void playLowWarning(int volume) {
    const int notes[]    = { 988, 784, 587 };
    const int durations[] = { 200, 200, 400 };
    playMelody(volume, notes, durations, 3);
}

// High alarm: urgent ascending major — C5 E5 G5 (repeated)
void playHighAlarm(int volume) {
    const int notes[]    = { 523, 659, 784,  523, 659, 784 };
    const int durations[] = { 150, 150, 300,  150, 150, 300 };
    playMelody(volume, notes, durations, 6);
}

// High warning: gentle ascending two-note — C5 E5
void playHighWarning(int volume) {
    const int notes[]    = { 523, 659 };
    const int durations[] = { 200, 350 };
    playMelody(volume, notes, durations, 2);
}

// No readings / stale data: two-tone attention chime — G5 D5
void playNoReadings(int volume) {
    const int notes[]    = { 784, 587 };
    const int durations[] = { 250, 400 };
    playMelody(volume, notes, durations, 2);
}

/* ── Check and fire alarms ─────────────────────────────────────── */

void checkAlarms(const Config &cfg, const NSinfo &ns, AlarmState &alarm) {
    // Calculate sensor age
    struct tm now;
    unsigned int sensorAgeMin = 999;
    if (getLocalTime(&now, 10)) {
        int ageSec = (int)difftime(mktime(&now), ns.sensTime);
        sensorAgeMin = (ageSec + 30) / 60;
    }

    bool loopErr = false;  // TODO: wire up if loop status is added later

    int level = alarmLevel(ns.sensSgv, cfg.snd_alarm, cfg.snd_warning,
                           cfg.snd_alarm_high, cfg.snd_warning_high,
                           sensorAgeMin, cfg.snd_no_readings, loopErr);

    if (level == ALARM_LEVEL_NORMAL)
        return;

    // Don't fire during snooze
    if (alarm.snoozeRemaining() > 0)
        return;

    // Don't fire too frequently
    if (!alarm.shouldFire(cfg.alarm_repeat))
        return;

    // Fire — each condition has a distinct melody
    switch (level) {
        case ALARM_LEVEL_LOW_ALARM:
            playLowAlarm(cfg.alarm_volume);
            break;
        case ALARM_LEVEL_HIGH_ALARM:
            playHighAlarm(cfg.alarm_volume);
            break;
        case ALARM_LEVEL_LOW_WARNING:
            playLowWarning(cfg.warning_volume);
            break;
        case ALARM_LEVEL_HIGH_WARNING:
            playHighWarning(cfg.warning_volume);
            break;
        case ALARM_LEVEL_NO_READINGS:
            playNoReadings(cfg.warning_volume);
            break;
        case ALARM_LEVEL_LOOP_ERROR:
            playHighAlarm(cfg.alarm_volume);
            break;
    }
    alarm.recordFired();
}

