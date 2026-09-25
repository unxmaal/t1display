/*  webconfig.cpp
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "webconfig.h"
#include "alerts.h"
#include "ns_config_parse.h"
#include "ns_pure_logic.h"
#include <WebServer.h>
#include <SD.h>
#include <M5Unified.h>
#include <WiFi.h>

static WebServer server(80);
static Config *cfgPtr = nullptr;

/* ── HTML helpers ──────────────────────────────────────────────── */

static String escapeHtml(const char *raw) {
    String s;
    for (const char *p = raw; *p; p++) {
        switch (*p) {
            case '&':  s += "&amp;";  break;
            case '<':  s += "&lt;";   break;
            case '>':  s += "&gt;";   break;
            case '"':  s += "&quot;"; break;
            case '\'': s += "&#39;";  break;
            default:   s += *p;       break;
        }
    }
    return s;
}

static String textInput(const char *label, const char *name, const char *value, int maxlen = 0) {
    String s = "<label>" + String(label) + "<br><input type='text' name='" + name + "' value='" + escapeHtml(value) + "'";
    if (maxlen > 0) s += " maxlength='" + String(maxlen) + "'";
    s += "></label><br>\n";
    return s;
}

static String numInput(const char *label, const char *name, int value) {
    return "<label>" + String(label) + "<br><input type='number' name='" + name + "' value='" + String(value) + "'></label><br>\n";
}

static String floatInput(const char *label, const char *name, float value) {
    return "<label>" + String(label) + "<br><input type='number' step='0.1' name='" + name + "' value='" + String(value, 1) + "'></label><br>\n";
}

static String passInput(const char *label, const char *name, const char *value) {
    return "<label>" + String(label) + "<br><input type='password' name='" + name + "' value='" + escapeHtml(value) + "'></label><br>\n";
}

/* ── Unit conversion (config stores mmol/L internally) ────────── */

static float toDisplay(float mmol, bool mgdl) {
    return mgdl ? mmol * MGDL_PER_MMOL : mmol;
}

/* ── GET / — serve config form ─────────────────────────────────── */

static void handleRoot() {
    const Config &c = *cfgPtr;
    String html = "<!DOCTYPE html><html><head>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>NightscoutMon Config</title>"
        "<style>"
        "body{font-family:sans-serif;max-width:600px;margin:0 auto;padding:16px;background:#1a1a1a;color:#eee}"
        "h1{color:#0cf}h2{color:#8cf;border-bottom:1px solid #444;padding-bottom:4px}"
        "label{display:block;margin:8px 0}"
        "input[type=text],input[type=number],input[type=password]{width:100%;padding:8px;box-sizing:border-box;"
        "background:#333;color:#eee;border:1px solid #555;border-radius:4px}"
        "button{padding:12px 24px;margin:8px 4px;border:none;border-radius:4px;font-size:16px;cursor:pointer}"
        ".save{background:#0a0;color:#fff}.reboot{background:#c00;color:#fff}"
        "</style></head><body>"
        "<h1>NightscoutMon Config</h1>"
        "<form method='POST' action='/save'>";

    // System status (live, read-only)
    html += "<h2>System Status</h2>";
    int batt = M5.Power.getBatteryLevel();
    int mv   = M5.Power.getBatteryVoltage();
    bool charging = M5.Power.isCharging();
    html += "<div style='background:#222;padding:12px;border-radius:6px;font-family:monospace;margin-bottom:12px'>";
    html += "Battery: " + String(batt) + "% (" + String(mv) + " mV)<br>";
    html += "Charging: " + String(charging ? "Yes" : "No") + "<br>";
    html += "Free heap: " + String(ESP.getFreeHeap() / 1024) + " KB<br>";
    html += "Uptime: " + String(millis() / 60000) + " min<br>";
    html += "IP: " + WiFi.localIP().toString() + "<br>";
    html += "</div>";

    // Alert test buttons
    html += "<h2>Test Alerts</h2>"
            "<div style='display:flex;flex-wrap:wrap;gap:8px;margin-bottom:12px'>"
            "<button onclick=\"fetch('/test?t=lw',{method:'POST'})\" style='background:#cc0;color:#000;padding:8px 12px;border:none;border-radius:4px;cursor:pointer'>Low Warning</button>"
            "<button onclick=\"fetch('/test?t=la',{method:'POST'})\" style='background:#c00;color:#fff;padding:8px 12px;border:none;border-radius:4px;cursor:pointer'>Low Alarm</button>"
            "<button onclick=\"fetch('/test?t=hw',{method:'POST'})\" style='background:#cc0;color:#000;padding:8px 12px;border:none;border-radius:4px;cursor:pointer'>High Warning</button>"
            "<button onclick=\"fetch('/test?t=ha',{method:'POST'})\" style='background:#c00;color:#fff;padding:8px 12px;border:none;border-radius:4px;cursor:pointer'>High Alarm</button>"
            "<button onclick=\"fetch('/test?t=nr',{method:'POST'})\" style='background:#888;color:#fff;padding:8px 12px;border:none;border-radius:4px;cursor:pointer'>No Readings</button>"
            "</div>";

    // Nightscout
    html += "<h2>Nightscout</h2>";
    html += textInput("Nightscout URL", "nightscout", c.url, 127);
    html += textInput("API Token", "token", c.token, 63);
    html += textInput("User Name", "name", c.userName, 31);
    html += textInput("Device Name", "device_name", c.deviceName, 31);

    // Display
    html += "<h2>Display</h2>";
    html += numInput("Show mg/dL (0=mmol, 1=mg/dL)", "show_mgdl", c.show_mgdl);
    html += numInput("Show Current Time (0/1)", "show_current_time", c.show_current_time);
    html += numInput("Default Page", "default_page", c.default_page);
    html += numInput("Info Line (0/1)", "info_line", c.info_line);
    html += numInput("Date Format", "date_format", c.date_format);
    html += numInput("Time Format", "time_format", c.time_format);
    html += numInput("SGV Only (0/1)", "sgv_only", c.sgv_only);
    html += numInput("Brightness 1", "brightness1", c.brightness1);
    html += numInput("Brightness 2", "brightness2", c.brightness2);
    html += numInput("Brightness 3", "brightness3", c.brightness3);

    // Thresholds — display in user's preferred unit
    bool mg = c.show_mgdl;
    const char *unit = mg ? "mg/dL" : "mmol/L";
    html += "<h2>Thresholds (" + String(unit) + ")</h2>";
    html += floatInput("Yellow Low", "yellow_low", toDisplay(c.yellow_low, mg));
    html += floatInput("Yellow High", "yellow_high", toDisplay(c.yellow_high, mg));
    html += floatInput("Red Low", "red_low", toDisplay(c.red_low, mg));
    html += floatInput("Red High", "red_high", toDisplay(c.red_high, mg));

    // Alarms
    html += "<h2>Alarms (" + String(unit) + ")</h2>";
    html += floatInput("Alarm (low)", "snd_alarm", toDisplay(c.snd_alarm, mg));
    html += floatInput("Warning (low)", "snd_warning", toDisplay(c.snd_warning, mg));
    html += floatInput("Alarm (high)", "snd_alarm_high", toDisplay(c.snd_alarm_high, mg));
    html += floatInput("Warning (high)", "snd_warning_high", toDisplay(c.snd_warning_high, mg));
    html += numInput("No Readings Alarm (min)", "snd_no_readings", c.snd_no_readings);
    html += numInput("Snooze Timeout (min)", "snooze_timeout", c.snooze_timeout);
    html += numInput("Alarm Repeat (min)", "alarm_repeat", c.alarm_repeat);
    html += numInput("Warning Volume (0-100)", "warning_volume", c.warning_volume);
    html += numInput("Alarm Volume (0-100)", "alarm_volume", c.alarm_volume);
    html += numInput("Loop Error Sound (0/1)", "snd_loop_error", c.snd_loop_error);

    // System
    html += "<h2>System</h2>";
    html += numInput("Time Zone (seconds offset)", "time_zone", c.timeZone);
    html += numInput("DST (seconds offset)", "dst", c.dst);
    html += numInput("Restart at Logged Errors (0=off)", "restart_at_logged_errors", c.restart_at_logged_errors);
    html += textInput("Restart at Time (HH:MM or NORES)", "restart_at_time", c.restart_at_time, 9);

    // WiFi
    html += "<h2>WiFi Networks</h2>";
    for (int i = 0; i < CFG_MAX_WLAN; i++) {
        String idx = String(i + 1);
        html += "<h3>WiFi " + idx + "</h3>";
        html += textInput(("SSID " + idx).c_str(), ("wlan_ssid_" + idx).c_str(), c.wlanssid[i], 63);
        html += passInput(("Password " + idx).c_str(), ("wlan_pass_" + idx).c_str(), c.wlanpass[i]);
    }

    // Buttons
    html += "<br><button type='submit' class='save'>Save to SD</button>"
            "</form>"
            "<form method='POST' action='/reboot' style='display:inline'>"
            "<button type='submit' class='reboot'>Reboot</button>"
            "</form>"
            "</body></html>";

    server.send(200, "text/html", html);
}

/* ── POST /save — update config and write to SD ───────────────── */

static void handleSave() {
    Config &c = *cfgPtr;

    int n = server.args();
    if (n > 128) n = 128;

    static String keys[128];
    static String vals[128];
    ConfigKV kv[128];
    for (int i = 0; i < n; i++) {
        keys[i] = server.argName(i);
        vals[i] = server.arg(i);
        kv[i].key = keys[i].c_str();
        kv[i].val = vals[i].c_str();
    }

    applyConfigForm(&c, kv, n);

    // Serialize to INI and write to SD
    char ini[4096];
    int wrote = serializeConfigINI(&c, ini, sizeof(ini));

    if (wrote <= 0) {
        server.send(500, "text/html",
            "<html><body style='background:#1a1a1a;color:#f88;font-family:sans-serif;padding:20px'>"
            "<h1>Error</h1><p>Failed to serialize config.</p>"
            "<a href='/' style='color:#0cf'>Back</a></body></html>");
        return;
    }

    bool sdOk = false;
    if (SD.begin(GPIO_NUM_4, SPI, 25000000)) {
        File f = SD.open("/M5NS.INI", FILE_WRITE);
        if (f) {
            f.write(reinterpret_cast<const uint8_t*>(ini), static_cast<size_t>(wrote));
            f.close();
            sdOk = true;
            Serial.printf("[WEBCONFIG] Wrote %d bytes to /M5NS.INI\n", wrote);
        }
        SD.end();
    }

    char cfgErr[48];
    formatConfigErrors(&c, cfgErr, sizeof(cfgErr));

    if (sdOk) {
        String page =
            "<html><body style='background:#1a1a1a;color:#0f0;font-family:sans-serif;padding:20px'>"
            "<h1>Saved</h1><p>Config written to SD card. Live config updated.</p>";
        if (cfgErr[0])
            page += "<p style='color:#ff0'>" + escapeHtml(cfgErr) +
                    ". Rejected values were replaced with defaults.</p>";
        page += "<p>Some changes (WiFi, timezone) require a reboot to take effect.</p>"
                "<a href='/' style='color:#0cf'>Back to config</a></body></html>";
        server.send(200, "text/html", page);
    } else {
        server.send(500, "text/html",
            "<html><body style='background:#1a1a1a;color:#f88;font-family:sans-serif;padding:20px'>"
            "<h1>Warning</h1><p>Live config updated but SD card write failed.</p>"
            "<p>Changes will be lost on reboot.</p>"
            "<a href='/' style='color:#0cf'>Back</a></body></html>");
    }
}

/* ── POST /reboot ──────────────────────────────────────────────── */

static void handleReboot() {
    server.send(200, "text/html",
        "<html><body style='background:#1a1a1a;color:#ff0;font-family:sans-serif;padding:20px'>"
        "<h1>Rebooting...</h1><p>Device will restart now.</p></body></html>");
    delay(500);
    ESP.restart();
}

/* ── GET /test — play an alert sound for testing ──────────────── */

static void handleTest() {
    const Config &c = *cfgPtr;
    String t = server.arg("t");
    if      (t == "lw") playLowWarning(c.warning_volume);
    else if (t == "la") playLowAlarm(c.alarm_volume);
    else if (t == "hw") playHighWarning(c.warning_volume);
    else if (t == "ha") playHighAlarm(c.alarm_volume);
    else if (t == "nr") playNoReadings(c.warning_volume);
    server.send(200, "text/plain", "OK");
}

/* ── Public API ────────────────────────────────────────────────── */

static bool requireAuth() {
    if (!configWebAuthEnabled(cfgPtr))
        return true;
    if (server.authenticate(cfgPtr->webUser, cfgPtr->webPass))
        return true;
    server.requestAuthentication();
    return false;
}

void setupWebConfig(Config *cfg) {
    cfgPtr = cfg;
    server.on("/", HTTP_GET, []() { if (requireAuth()) handleRoot(); });
    server.on("/save", HTTP_POST, []() { if (requireAuth()) handleSave(); });
    server.on("/reboot", HTTP_POST, []() { if (requireAuth()) handleReboot(); });
    server.on("/test", HTTP_POST, []() { if (requireAuth()) handleTest(); });
    server.begin();
    Serial.printf("[WEBCONFIG] Server started on port 80\n");
}

void handleWebConfig() {
    server.handleClient();
}
