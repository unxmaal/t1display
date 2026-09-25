/*  webconfig.cpp
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "webconfig.h"
#include "alerts.h"
#include "ns_config_parse.h"
#include "ns_pure_logic.h"
#include "ns_display_model.h"
#include <time.h>
#include <WebServer.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

static WebServer  server(80);
static WebShared *shared = nullptr;
static Config     view;

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

static String passInput(const char *label, const char *name, const char *stored) {
    bool isSet = stored[0] != '\0';
    String s = "<label>" + String(label) + "<br><input type='password' name='" + name +
               "' value='' autocomplete='new-password' placeholder='" +
               (isSet ? "set - leave blank to keep" : "not set") + "'></label>";
    if (isSet)
        s += "<label><input type='checkbox' name='clear_" + String(name) + "' value='1'> clear</label>";
    return s + "<br>\n";
}

/* ── Unit conversion (config stores mmol/L internally) ────────── */

static float toDisplay(float mmol, bool mgdl) {
    return mgdl ? mmol * MGDL_PER_MMOL : mmol;
}

/* ── GET / — serve config form ─────────────────────────────────── */

static void handleRoot() {
    const Config &c = view;
    String title = "t1display " + escapeHtml(c.deviceName);
    String html = "<!DOCTYPE html><html><head>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>" + title + "</title>"
        "<style>"
        "body{font-family:sans-serif;max-width:600px;margin:0 auto;padding:16px;background:#1a1a1a;color:#eee}"
        "h1{color:#0cf}h2{color:#8cf;border-bottom:1px solid #444;padding-bottom:4px}"
        "label{display:block;margin:8px 0}"
        "input[type=text],input[type=number],input[type=password]{width:100%;padding:8px;box-sizing:border-box;"
        "background:#333;color:#eee;border:1px solid #555;border-radius:4px}"
        "button{padding:12px 24px;margin:8px 4px;border:none;border-radius:4px;font-size:16px;cursor:pointer}"
        ".save{background:#0a0;color:#fff}.reboot{background:#c00;color:#fff}"
        "</style></head><body>"
        "<h1>" + title + "</h1>"
        "<form method='POST' action='/save'>";

    // System status (live, read-only)
    html += "<h2>System Status</h2>";
    PowerStatus power = shared->power->snapshot();
    html += "<div style='background:#222;padding:12px;border-radius:6px;font-family:monospace;margin-bottom:12px'>";
    html += "Battery: " + String(power.pct) + "% (" + String(power.mv) + " mV)<br>";
    html += "Charging: " + String(power.charging ? "Yes" : "No") + "<br>";
    html += "Free heap: " + String(ESP.getFreeHeap() / 1024) + " KB<br>";
    html += "Uptime: " + String(millis() / 60000) + " min<br>";
    html += "IP: " + WiFi.localIP().toString() + "<br>";
    html += "</div>";

    // Error log
    NsErrorLog log = shared->errors->snapshot();
    int held = nsErrorLogHeld(&log);
    html += "<h2>Error Log</h2><div style='background:#222;padding:12px;border-radius:6px;"
            "font-family:monospace;margin-bottom:12px'>";
    if (held == 0)
        html += "no errors in log<br>";
    for (int i = 0; i < held; i++) {
        time_t ts = (time_t)nsErrorLogTimeAt(&log, i);
        struct tm local;
        const struct tm *et = ts ? localtime_r(&ts, &local) : NULL;
        char date[16], desc[32];
        formatLogDate(date, sizeof(date), et, c.date_format, c.time_format);
        describeErrorCode(nsErrorLogCodeAt(&log, i), desc, sizeof(desc));
        html += String(date) + "&nbsp;&nbsp;" + escapeHtml(desc) + "<br>";
    }
    html += "Total errors: " + String((unsigned long)log.total) + "</div>";

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
    html += passInput("API Token", "token", c.token);
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

static void sendPage(int code, const char *colour, const String &body) {
    server.send(code, "text/html",
        String("<html><body style='background:#1a1a1a;color:") + colour +
        ";font-family:sans-serif;padding:20px'>" + body +
        "<a href='/' style='color:#0cf'>Back to config</a></body></html>");
}

static void handleSave() {
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

    applyConfigForm(&view, kv, n);

    if (!shared->save->submit(view)) {
        sendPage(503, "#ff0", "<h1>Busy</h1><p>Another save is in progress. Try again.</p>");
        return;
    }

    SaveResult r;
    bool done = false;
    for (int i = 0; i < 100 && !(done = shared->save->collect(&r)); i++) {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (!done) {
        sendPage(504, "#ff0", "<h1>Pending</h1><p>The device has not confirmed the save yet. "
                              "Reload the config page to check.</p>");
    } else if (r.wrote <= 0) {
        sendPage(500, "#f88", "<h1>Error</h1><p>Failed to serialize config.</p>");
    } else if (!r.sdOk) {
        sendPage(500, "#f88", "<h1>Warning</h1><p>Live config updated but SD card write failed.</p>"
                              "<p>Changes will be lost on reboot.</p>");
    } else {
        String body = "<h1>Saved</h1><p>Config written to SD card. Live config updated.</p>";
        if (r.errors[0])
            body += "<p style='color:#ff0'>" + escapeHtml(r.errors) +
                    ". Rejected values were replaced with defaults.</p>";
        body += "<p>Some changes (WiFi, timezone) require a reboot to take effect.</p>";
        sendPage(200, "#0f0", body);
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
    String t = server.arg("t");
    if      (t == "lw") requestTestSound(ALARM_SOUND_LOW_WARNING);
    else if (t == "la") requestTestSound(ALARM_SOUND_LOW_ALARM);
    else if (t == "hw") requestTestSound(ALARM_SOUND_HIGH_WARNING);
    else if (t == "ha") requestTestSound(ALARM_SOUND_HIGH_ALARM);
    else if (t == "nr") requestTestSound(ALARM_SOUND_NO_READINGS);
    server.send(200, "text/plain", "OK");
}

/* ── Public API ────────────────────────────────────────────────── */

static bool requireAuth() {
    view = shared->cfg->snapshot();
    if (!configWebAuthEnabled(&view))
        return true;
    if (server.authenticate(view.webUser, view.webPass))
        return true;
    server.requestAuthentication();
    return false;
}

void setupWebConfig(WebShared *state) {
    shared = state;
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
