/*  ns_melody.h — Non-blocking melody sequencing
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NS_MELODY_H
#define NS_MELODY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MELODY_NOTE_GAP_MS 60

struct MelodySequencer {
    const int    *notes;
    const int    *durations;
    int           count;
    int           next;
    uint32_t dueMs;
    bool          active;
};

void melodyInit(MelodySequencer *m);

void melodyStart(MelodySequencer *m, const int *notes, const int *durations,
                 int count, uint32_t nowMs);

bool melodyActive(const MelodySequencer *m);

/** Index of the note to emit now, or -1 when nothing is due. */
int melodyNextNote(MelodySequencer *m, uint32_t nowMs);

int melodyFreqAt(const MelodySequencer *m, int index);
int melodyDurationAt(const MelodySequencer *m, int index);

#ifdef __cplusplus
}
#endif

#endif
