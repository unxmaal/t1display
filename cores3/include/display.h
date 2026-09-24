/*  display.h
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CORES3_DISPLAY_H
#define CORES3_DISPLAY_H

#include "config.h"

/* Number of pages */
#define PAGE_GLUCOSE  0
#define PAGE_STATUS   1
#define NUM_PAGES     2

/**
 * Allocate offscreen canvas for flicker-free rendering.
 * Call once after M5.begin(). Falls back to direct draw on failure.
 */
void initCanvas();

/**
 * Draw the large glucose screen.
 * Shows: time, delta, huge glucose number, trend arrow, status line.
 */
void drawGlucosePage(const Config &cfg, const NSinfo &ns, const ErrorLog &errLog);

/**
 * Draw the error log / status screen.
 * Shows: error history, heap, uptime, IP, version.
 */
void drawStatusPage(const Config &cfg, const NSinfo &ns, const ErrorLog &errLog);

/**
 * Draw the current page based on page number.
 */
void drawPage(int page, const Config &cfg, const NSinfo &ns, const ErrorLog &errLog);

/**
 * Draw the trend arrow at (x, y) with given color.
 * angle: degrees from horizontal, 0=flat, negative=up, positive=down.
 * 180 = no arrow (hidden).
 */
void drawArrow(int x, int y, int size, int angle, uint16_t color);

#endif // CORES3_DISPLAY_H
