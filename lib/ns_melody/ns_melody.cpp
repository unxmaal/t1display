/*  ns_melody.cpp — Non-blocking melody sequencing
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ns_melody.h"

#include <string.h>

void melodyInit(MelodySequencer *m) {
    memset(m, 0, sizeof(*m));
}

void melodyStart(MelodySequencer *m, const int *notes, const int *durations,
                 int count, uint32_t nowMs) {
    if (!notes || !durations || count <= 0) {
        melodyInit(m);
        return;
    }
    m->notes     = notes;
    m->durations = durations;
    m->count     = count;
    m->next      = 0;
    m->dueMs     = nowMs;
    m->active    = true;
}

bool melodyActive(const MelodySequencer *m) {
    return m->active;
}

int melodyNextNote(MelodySequencer *m, uint32_t nowMs) {
    if (!m->active)
        return -1;
    if ((int32_t)(nowMs - m->dueMs) < 0)
        return -1;

    int idx = m->next;
    m->dueMs = nowMs + (uint32_t)m->durations[idx] + MELODY_NOTE_GAP_MS;
    m->next++;
    if (m->next >= m->count)
        m->active = false;
    return idx;
}

int melodyFreqAt(const MelodySequencer *m, int index) {
    if (index < 0 || index >= m->count) return 0;
    return m->notes[index];
}

int melodyDurationAt(const MelodySequencer *m, int index) {
    if (index < 0 || index >= m->count) return 0;
    return m->durations[index];
}
