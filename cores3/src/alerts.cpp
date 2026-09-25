/*  alerts.cpp
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "alerts.h"
#include "ns_pure_logic.h"
#include "ns_melody.h"

#include <M5Unified.h>
#include <time.h>

/* ── AlarmState ────────────────────────────────────────────────── */

void AlarmState::snooze(uint32_t nowMs, int level, int timeout_min) {
    alarmScheduleSnooze(&sched, nowMs, level, timeout_min);
}

uint32_t AlarmState::snoozeRemaining(uint32_t nowMs) const {
    return alarmSnoozeRemainingSec(&sched, nowMs);
}

/* ── Sound helpers ─────────────────────────────────────────────── */

// Scale config volume (0-100) to speaker range (0-255)
static int scaleVolume(int vol) {
    return (vol * 255) / 100;
}

static MelodySequencer melody;

static void playMelody(int volume, const int *notes, const int *durations, int count) {
    M5.Speaker.setVolume(scaleVolume(volume));
    melodyStart(&melody, notes, durations, count, millis());
}

void serviceAlerts() {
    if (!melodyActive(&melody))
        return;
    uint32_t nowMs = millis();
    int idx = melodyNextNote(&melody, nowMs);
    if (idx >= 0)
        M5.Speaker.tone(melodyFreqAt(&melody, idx), melodyDurationAt(&melody, idx));
}

// Low alarm: descending tritone + chromatic fall — psychoacoustically urgent
// B5→F5 (tritone), then chromatic descent E5→Eb5→D5→Db5 (falling sensation)
// Repeated twice with shorter gaps for urgency
void playLowAlarm(int volume) {
    static const int notes[]    = { 988, 698,  659, 622, 587, 554,
                             988, 698,  659, 622, 587, 554 };
    static const int durations[] = { 120, 200,  100, 100, 100, 250,
                              120, 200,  100, 100, 100, 250 };
    playMelody(volume, notes, durations, 12);
}

// Low warning: gentle descending three-note — B5 G5 D5
void playLowWarning(int volume) {
    static const int notes[]    = { 988, 784, 587 };
    static const int durations[] = { 200, 200, 400 };
    playMelody(volume, notes, durations, 3);
}

// High alarm: urgent ascending major — C5 E5 G5 (repeated)
void playHighAlarm(int volume) {
    static const int notes[]    = { 523, 659, 784,  523, 659, 784 };
    static const int durations[] = { 150, 150, 300,  150, 150, 300 };
    playMelody(volume, notes, durations, 6);
}

// High warning: gentle ascending two-note — C5 E5
void playHighWarning(int volume) {
    static const int notes[]    = { 523, 659 };
    static const int durations[] = { 200, 350 };
    playMelody(volume, notes, durations, 2);
}

// No readings / stale data: two-tone attention chime — G5 D5
void playNoReadings(int volume) {
    static const int notes[]    = { 784, 587 };
    static const int durations[] = { 250, 400 };
    playMelody(volume, notes, durations, 2);
}

/* ── Check and fire alarms ─────────────────────────────────────── */

int currentAlarmLevel(const Config &cfg, const NSinfo &ns) {
    struct tm now;
    long now_sec = getLocalTime(&now, 10) ? (long)mktime(&now) : 0;
    int sensorAgeMin = sensorAgeMinutes(now_sec, (long)ns.sensTime);

    return alarmLevel(ns.sensSgv, cfg.snd_alarm, cfg.snd_warning,
                      cfg.snd_alarm_high, cfg.snd_warning_high,
                      (unsigned int)sensorAgeMin, cfg.snd_no_readings, false);
}

void checkAlarms(const Config &cfg, const NSinfo &ns, AlarmState &alarm) {
    int level = currentAlarmLevel(cfg, ns);
    uint32_t nowMs = millis();

    if (!alarmShouldFire(&alarm.sched, nowMs, level, cfg.alarm_repeat))
        return;

    switch (alarmSound(level)) {
        case ALARM_SOUND_LOW_ALARM:
            playLowAlarm(cfg.alarm_volume);
            break;
        case ALARM_SOUND_HIGH_ALARM:
            playHighAlarm(cfg.alarm_volume);
            break;
        case ALARM_SOUND_LOW_WARNING:
            playLowWarning(cfg.warning_volume);
            break;
        case ALARM_SOUND_HIGH_WARNING:
            playHighWarning(cfg.warning_volume);
            break;
        case ALARM_SOUND_NO_READINGS:
            playNoReadings(cfg.warning_volume);
            break;
    }
    alarmRecordFired(&alarm.sched, nowMs);
}
