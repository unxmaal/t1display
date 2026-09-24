/*  display.cpp
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "display.h"
#include "ns_display_model.h"
#include "ns_pure_logic.h"

#include <M5Unified.h>
#include <WiFi.h>
#include <stdio.h>
#include <math.h>

/* ── Offscreen canvas for flicker-free rendering ─────────────── */

static M5Canvas canvas(&M5.Display);
static bool canvasReady = false;

void initCanvas() {
    canvas.setColorDepth(16);
    const void *buf = canvas.createSprite(320, 240);
    canvasReady = (buf != nullptr);
    if (!canvasReady)
        Serial.println("[DISPLAY] Canvas alloc failed, using direct draw");
    else
        Serial.println("[DISPLAY] Canvas allocated (320x240 in PSRAM)");
}

/* ── GFX target: canvas if available, else direct display ────── */

static LGFX_Device& directGfx() { return M5.Display; }

static LovyanGFX& gfx() {
    if (canvasReady) return canvas;
    return directGfx();
}

/* ── Color mapping: model COLOR_* → TFT hardware colors ──────── */

static uint16_t mapColor(int c) {
    switch (c) {
        case COLOR_BLACK:     return TFT_BLACK;
        case COLOR_GREEN:     return TFT_GREEN;
        case COLOR_YELLOW:    return TFT_YELLOW;
        case COLOR_RED:       return TFT_RED;
        case COLOR_WHITE:     return TFT_WHITE;
        case COLOR_LIGHTGREY: return TFT_LIGHTGREY;
        default:              return TFT_WHITE;
    }
}

/* ── Font mapping: model FONT_* → actual font objects ─────────── */

static const lgfx::GFXfont* mapFont(int f) {
    switch (f) {
        case FONT_SANS_BOLD_24: return &FreeSansBold24pt7b;
        case FONT_SANS_BOLD_18: return &FreeSansBold18pt7b;
        case FONT_SANS_BOLD_12: return &FreeSansBold12pt7b;
        case FONT_SANS_9:       return &FreeSans9pt7b;
        case FONT_MONO_9:       return &FreeMono9pt7b;
        default:                return &FreeSans9pt7b;
    }
}

/* ── Trend arrow ───────────────────────────────────────────────── */

void drawArrow(int x, int y, int size, int angle, uint16_t color) {
    if (angle == 180)
        return;  // no arrow for unknown direction

    auto &g = gfx();

    float rad = (angle + 85) * M_PI / 180.0f;
    float cosA = cosf(rad);
    float sinA = sinf(rad);

    // Arrow shaft endpoint
    int tipX = x + (int)(size * 2 * cosA);
    int tipY = y + (int)(size * 2 * sinA);

    // Draw shaft
    g.drawLine(x, y, tipX, tipY, color);

    // Arrowhead — two lines from tip at ±30 degrees
    float headLen = size * 0.8f;
    for (int sign = -1; sign <= 1; sign += 2) {
        float headRad = rad + sign * (M_PI / 6.0f) + M_PI;
        int hx = tipX + (int)(headLen * cosf(headRad));
        int hy = tipY + (int)(headLen * sinf(headRad));
        g.drawLine(tipX, tipY, hx, hy, color);
    }
}

/* ── Battery icon ──────────────────────────────────────────────── */

static void drawBattery(int x, int y, int pct) {
    auto &g = gfx();

    uint16_t color = TFT_GREEN;
    if (pct < 0) {
        color = TFT_LIGHTGREY;  // unknown
        pct = 0;
    } else if (pct < 25) {
        color = TFT_RED;
    } else if (pct < 50) {
        color = TFT_YELLOW;
    }

    // Battery outline
    g.drawRect(x, y, 20, 10, TFT_LIGHTGREY);
    g.fillRect(x + 20, y + 2, 2, 6, TFT_LIGHTGREY);

    // Fill
    int fillW = (16 * pct) / 100;
    if (fillW > 0)
        g.fillRect(x + 2, y + 2, fillW, 6, color);
}

/* ── Page 0: Large glucose ─────────────────────────────────────── */

void drawGlucosePage(const Config &cfg, const NSinfo &ns, const ErrorLog &errLog,
                     int snoozeRemainingSec) {
    // Build the display model — all decisions happen here
    struct tm now;
    long now_sec = 0;
    int time_hour = ns.sensTm.tm_hour;
    int time_min  = ns.sensTm.tm_min;

    if (getLocalTime(&now, 10)) {
        now_sec = mktime(&now);
        if (cfg.show_current_time) {
            time_hour = now.tm_hour;
            time_min  = now.tm_min;
        }
    }

    int batteryPct = M5.Power.getBatteryLevel();

    // Compute alarm level for the model
    int sensorAgeMin = sensorAgeMinutes((long)now_sec, (long)ns.sensTime);
    int level = alarmLevel(ns.sensSgv, cfg.snd_alarm, cfg.snd_warning,
                           cfg.snd_alarm_high, cfg.snd_warning_high,
                           (unsigned int)sensorAgeMin, cfg.snd_no_readings, false);


    GlucosePageModel model;
    buildGlucoseModel(&model,
        ns.sensSgv, ns.sensSgvMgDl, cfg.show_mgdl,
        ns.sensDir, ns.arrowAngle,
        ns.delta_display,
        time_hour, time_min,
        now_sec, (long)ns.sensTime,
        cfg.yellow_low, cfg.yellow_high, cfg.red_low, cfg.red_high,
        level, snoozeRemainingSec,
        batteryPct, nsErrorLogHasActiveFault(&errLog) ? 1 : 0);

    // ── Render from model ─────────────────────────────────────────

    auto &g = gfx();

    g.fillRect(0, 0, 320, 240, TFT_BLACK);

    // Top bar: time (left), delta (right)
    g.setTextSize(1);
    g.setTextDatum(TL_DATUM);
    g.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    g.setFont(&FreeSansBold24pt7b);
    g.drawString(model.time_str, 0, 0);

    g.setTextColor(TFT_WHITE, TFT_BLACK);
    g.drawString(model.delta_str, 180, 0);

    // Center: glucose value — baseline aligned with battery indicator top
    g.setTextColor(mapColor(model.glucose_color), TFT_BLACK);
    g.setTextDatum(BC_DATUM);
    g.setTextSize(3);
    g.setFont(mapFont(model.glucose_font));
    g.drawString(model.glucose_str, 160, 224);
    g.setTextSize(1);

    // Trend arrow
    int arrowY = 0;
    if (model.arrow_angle >= 45)
        arrowY = 4;
    else if (model.arrow_angle > -45)
        arrowY = 18;
    else
        arrowY = 30;
    drawArrow(280, arrowY, 10, model.arrow_angle, mapColor(model.arrow_color));

    // Sensor staleness
    if (model.show_age) {
        g.setTextDatum(TR_DATUM);
        g.setTextColor(mapColor(model.age_color), TFT_BLACK);
        g.setFont(&FreeSans9pt7b);
        g.drawString(model.age_str, 318, 45);
    }

    // Bottom bar
    drawBattery(296, 226, model.battery_pct);

    if (model.show_error_badge) {
        g.setTextDatum(TL_DATUM);
        g.setTextColor(TFT_RED, TFT_BLACK);
        g.setFont(&FreeSans9pt7b);
        g.drawString("!", 2, 224);
    }

    // Alarm bar
    if (model.show_alarm_bar) {
        g.fillRect(0, 220, 320, 20, mapColor(model.alarm_bar_bg));
        if (model.alarm_bar_text[0] != '\0') {
            g.setTextDatum(MC_DATUM);
            g.setTextColor(mapColor(model.alarm_bar_fg),
                                    mapColor(model.alarm_bar_bg));
            g.setFont(&FreeSansBold12pt7b);
            g.drawString(model.alarm_bar_text, 160, 230);
        }
    }
}

/* ── Page 1: Error log / status ────────────────────────────────── */

void drawStatusPage(const Config &cfg, const NSinfo &ns, const ErrorLog &errLog) {
    (void)cfg; (void)ns;

    // Prepare error data for the model
    int codes[STATUS_MAX_ERRORS];
    char dates[STATUS_MAX_ERRORS][16];
    int displayCount = nsErrorLogHeld(&errLog);
    if (displayCount > STATUS_MAX_ERRORS)
        displayCount = STATUS_MAX_ERRORS;

    for (int i = 0; i < displayCount; i++) {
        codes[i] = nsErrorLogCodeAt(&errLog, i);
        time_t ts = (time_t)nsErrorLogTimeAt(&errLog, i);
        struct tm *et = ts ? localtime(&ts) : NULL;
        snprintf(dates[i], sizeof(dates[i]), "%02d.%02d.%02d:%02d",
                 et ? et->tm_mday : 0,
                 et ? et->tm_mon + 1 : 0,
                 et ? et->tm_hour : 0,
                 et ? et->tm_min : 0);
    }

    char ipStr[32];
    IPAddress ip = WiFi.localIP();
    snprintf(ipStr, sizeof(ipStr), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);

    StatusPageModel model;
    buildStatusModel(&model,
        codes, dates, displayCount, (int)errLog.total,
        ESP.getFreeHeap(), millis(),
        ipStr, "CoreS3",
        M5.Power.getBatteryLevel());

    // ── Render from model ─────────────────────────────────────────

    auto &g = gfx();

    g.fillScreen(TFT_BLACK);
    g.setTextDatum(TL_DATUM);
    g.setTextSize(1);

    // Header
    g.setFont(&FreeMono9pt7b);
    g.setTextColor(TFT_WHITE, TFT_BLACK);
    g.drawString("Date  Time  Error Log", 0, 0);

    if (model.display_count == 0) {
        g.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        g.drawString("no errors in log", 0, 20);
    } else {
        for (int i = 0; i < model.display_count; i++) {
            g.setTextColor(TFT_WHITE, TFT_BLACK);
            g.drawString(model.errors[i].date_str, 0, 20 + i * 18);

            g.setTextColor(mapColor(model.errors[i].color), TFT_BLACK);
            g.drawString(model.errors[i].desc_str, 132, 20 + i * 18);
        }

        g.setTextColor(TFT_WHITE, TFT_BLACK);
        char countStr[32];
        snprintf(countStr, sizeof(countStr), "Total errors %d", model.error_count);
        g.drawString(countStr, 0, 20 + model.display_count * 18);
    }

    // System info
    int infoY = 20 + 7 * 18;
    g.setTextColor(TFT_WHITE, TFT_BLACK);
    g.drawString(model.heap_str, 0, infoY);
    g.drawString(model.uptime_str, 0, infoY + 18);
    g.drawString(model.ip_str, 0, infoY + 36);
    g.drawString(model.version_str, 0, infoY + 54);

    drawBattery(296, 226, model.battery_pct);
}

/* ── Page dispatcher ───────────────────────────────────────────── */

void drawPage(int page, const Config &cfg, const NSinfo &ns, const ErrorLog &errLog,
              int snoozeRemainingSec) {
    switch (page) {
        case PAGE_GLUCOSE: drawGlucosePage(cfg, ns, errLog, snoozeRemainingSec); break;
        case PAGE_STATUS:  drawStatusPage(cfg, ns, errLog); break;
        default:           drawGlucosePage(cfg, ns, errLog, snoozeRemainingSec); break;
    }

    // Flush canvas to display atomically (zero flicker)
    if (canvasReady) {
        M5.Display.startWrite();
        canvas.pushSprite(0, 0);
        M5.Display.endWrite();
    }
}
