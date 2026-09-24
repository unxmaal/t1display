/*  main.cpp
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <SD.h>

#include "config.h"
#include "nightscout.h"
#include "display.h"
#include "alerts.h"
#include "ota.h"
#include "webconfig.h"
#include "ns_config_parse.h"

/* ── Globals ───────────────────────────────────────────────────── */

static Config     cfg;
static NSinfo     ns;
static ErrorLog   errLog;
static AlarmState alarmState;

static WiFiMulti  wifiMulti;

static int  currentPage       = PAGE_GLUCOSE;
static int  brightnessLevel   = 0;
static int  brightnessValues[3];
static unsigned long lastNsCheck = 0;
static int  pollCount         = 0;

static const char *ntpServer = "pool.ntp.org";

/* ── Config loading from SD card INI ──────────────────────────── */

static bool loadConfigFromSD() {
    if (!SD.begin(GPIO_NUM_4, SPI, 25000000)) {
        Serial.println("SD card mount failed");
        return false;
    }

    File f = SD.open("/M5NS.INI", FILE_READ);
    if (!f) {
        Serial.println("M5NS.INI not found on SD");
        SD.end();
        return false;
    }

    size_t fileSize = f.size();
    if (fileSize == 0 || fileSize > 8192) {
        Serial.printf("M5NS.INI bad size: %u\n", (unsigned)fileSize);
        f.close();
        SD.end();
        return false;
    }

    char *buf = new char[fileSize + 1];
    size_t bytesRead = f.read(reinterpret_cast<uint8_t*>(buf), fileSize);
    f.close();
    SD.end();

    buf[bytesRead] = '\0';

    int parsed = parseConfigBuffer(buf, bytesRead, &cfg);
    delete[] buf;

    Serial.printf("Parsed %d config values from M5NS.INI\n", parsed);
    return parsed > 0;
}

/* ── WiFi ──────────────────────────────────────────────────────── */

static void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    int apCount = 0;
    for (int i = 0; i < CFG_MAX_WLAN; i++) {
        if (cfg.wlanssid[i][0] != '\0') {
            wifiMulti.addAP(cfg.wlanssid[i], cfg.wlanpass[i]);
            apCount++;
        }
    }

    // Wokwi virtual network — harmless on real hardware (AP won't exist)
    wifiMulti.addAP("Wokwi-GUEST", "");

    if (apCount == 0) {
        Serial.println("[WIFI] No user SSIDs configured (Wokwi-GUEST added as fallback)");
    }

    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setFont(&FreeSans9pt7b);
    M5.Display.setTextDatum(TL_DATUM);
    M5.Display.drawString("Connecting WiFi...", 10, 100);

    Serial.printf("[WIFI] Connecting (%d APs)...\n", apCount);
    int attempts = 0;
    while (wifiMulti.run() != WL_CONNECTED && attempts < 10) {
        delay(500);
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WIFI] Connected, IP: %s\n",
                   WiFi.localIP().toString().c_str());
        M5.Display.fillScreen(TFT_BLACK);
        M5.Display.drawString("WiFi connected", 10, 100);
        M5.Display.drawString(WiFi.localIP().toString().c_str(), 10, 120);
        delay(1000);

        // NTP time sync
        Serial.println("[NTP] Syncing time...");
        configTime(cfg.timeZone, cfg.dst, ntpServer, "time.nist.gov", "time.google.com");
        struct tm timeinfo;
        for (int i = 0; i < 10; i++) {
            if (getLocalTime(&timeinfo, 10)) {
                Serial.println("[NTP] Time synced");
                break;
            }
            delay(1000);
        }
    } else {
        Serial.println("[WIFI] Connection failed");
        M5.Display.fillScreen(TFT_BLACK);
        M5.Display.setTextColor(TFT_RED, TFT_BLACK);
        M5.Display.drawString("WiFi FAILED", 10, 100);
        delay(1000);
    }
}

/* ── Brightness ────────────────────────────────────────────────── */

static void cycleBrightness() {
    brightnessLevel = (brightnessLevel + 1) % 3;
    M5.Display.setBrightness(brightnessValues[brightnessLevel]);
}

/* ── Nightscout polling ────────────────────────────────────────── */

static void pollNightscout() {
    if (millis() - lastNsCheck < 15000)
        return;
    lastNsCheck = millis();

    // Only poll if data is stale (>5 min) and we've waited a few cycles
    struct tm now;
    int sensorAgeSec = 86400;
    if (getLocalTime(&now, 10))
        sensorAgeSec = (int)difftime(mktime(&now), ns.sensTime);

    if (sensorAgeSec > 305 && pollCount > 3) {
        pollCount = 0;
        readNightscout(cfg, ns, errLog);
    }
    pollCount++;

    drawPage(currentPage, cfg, ns, errLog, (int)alarmState.snoozeRemaining(millis()));
}

/* ── Setup ─────────────────────────────────────────────────────── */

void setup() {
    auto m5cfg = M5.config();
    M5.begin(m5cfg);

    Serial.begin(115200);
    Serial.println("[BOOT] M5Unified initialized");
    Serial.printf("[BOOT] Board: %d\n", M5.getBoard());

    // Speaker
    M5.Speaker.begin();
    M5.Speaker.setVolume(64);

    // Load config: defaults first, then overlay from SD INI
    configDefaults(&cfg);
    Serial.println("[CONFIG] Defaults loaded");

    if (loadConfigFromSD()) {
        Serial.println("[CONFIG] SD config loaded OK");
    } else {
        Serial.println("[CONFIG] SD failed, using defaults");
        // Fallback for testing without SD card — reads from build flags
        #ifdef TEST_NS_URL
        strlcpy(cfg.url, TEST_NS_URL, sizeof(cfg.url));
        #endif
        #ifdef TEST_NS_TOKEN
        strlcpy(cfg.token, TEST_NS_TOKEN, sizeof(cfg.token));
        #endif
    }

    Serial.printf("[CONFIG] url=%s\n", cfg.url);
    Serial.printf("[CONFIG] device=%s\n", cfg.deviceName);
    Serial.printf("[CONFIG] brightness=%d/%d/%d\n",
               cfg.brightness1, cfg.brightness2, cfg.brightness3);
    Serial.printf("[CONFIG] thresholds yellow=%.1f-%.1f red=%.1f-%.1f\n",
               cfg.yellow_low, cfg.yellow_high, cfg.red_low, cfg.red_high);

    brightnessValues[0] = cfg.brightness1;
    brightnessValues[1] = cfg.brightness2;
    brightnessValues[2] = cfg.brightness3;
    M5.Display.setBrightness(brightnessValues[brightnessLevel]);

    initCanvas();

    // Power debug
    Serial.printf("[POWER] Battery: %d%%, Voltage: %dmV, Charging: %s\n",
                  M5.Power.getBatteryLevel(),
                  M5.Power.getBatteryVoltage(),
                  M5.Power.isCharging() ? "yes" : "no");

    currentPage = cfg.default_page;

    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setFont(&FreeSansBold18pt7b);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.drawString(cfg.deviceName, 160, 60);
    M5.Display.setFont(&FreeSans9pt7b);
    M5.Display.drawString("CoreS3", 160, 100);
    Serial.println("[DISPLAY] Splash screen drawn");

    connectWiFi();

    // OTA updates (only useful once WiFi is connected)
    if (WiFi.status() == WL_CONNECTED) {
        setupOTA(cfg.deviceName);
        setupWebConfig(&cfg);
    }

    // Initial fetch (will fail without WiFi — that's OK)
    Serial.println("[NS] Initial Nightscout fetch...");
    int nsRc = readNightscout(cfg, ns, errLog);
    Serial.printf("[NS] Result: %d, errors logged: %d\n", nsRc, errLog.count);

    Serial.println("[DISPLAY] Drawing initial page...");
    Serial.flush();
    drawPage(currentPage, cfg, ns, errLog, (int)alarmState.snoozeRemaining(millis()));
    Serial.printf("[DISPLAY] Page %d drawn (glucose=%.1f mmol, dir=%s)\n",
                  currentPage, ns.sensSgv, ns.sensDir);
    Serial.flush();
    Serial.println("[BOOT] Setup complete, entering loop");
    Serial.flush();
}

/* ── Loop ──────────────────────────────────────────────────────── */

void loop() {
    M5.update();

    // CoreS3 touch → virtual button mapping (touch panel covers display only)
    auto t = M5.Touch.getDetail();
    if (t.isPressed() && t.y >= 200) {
        int zone = t.x / 107;  // 0=left, 1=mid, 2=right (320/3)
        M5.BtnA.setRawState(t.wasPressed() ? 0 : 0, zone == 0);
        M5.BtnB.setRawState(t.wasPressed() ? 0 : 0, zone == 1);
        M5.BtnC.setRawState(t.wasPressed() ? 0 : 0, zone == 2);
    } else if (t.wasReleased()) {
        M5.BtnA.setRawState(0, false);
        M5.BtnB.setRawState(0, false);
        M5.BtnC.setRawState(0, false);
    }

    handleOTA();
    handleWebConfig();

    // Button A (left touch zone): cycle brightness
    if (M5.BtnA.wasPressed()) {
        Serial.println("[BTN] A pressed");
        cycleBrightness();
    }

    // Button B (middle): snooze
    if (M5.BtnB.wasPressed()) {
        Serial.println("[BTN] B pressed");
        alarmState.snooze(millis(), currentAlarmLevel(cfg, ns), cfg.snooze_timeout);
        drawPage(currentPage, cfg, ns, errLog, (int)alarmState.snoozeRemaining(millis()));
    }

    // Button C (right): toggle page
    if (M5.BtnC.wasPressed()) {
        Serial.println("[BTN] C pressed");
        currentPage = (currentPage + 1) % NUM_PAGES;
        drawPage(currentPage, cfg, ns, errLog, (int)alarmState.snoozeRemaining(millis()));
    }

    // Poll Nightscout + redraw
    pollNightscout();

    // Check alarms
    checkAlarms(cfg, ns, alarmState);

    // Auto-restart on too many errors
    if (cfg.restart_at_logged_errors > 0 &&
        errLog.count >= cfg.restart_at_logged_errors) {
        ESP.restart();
    }

    delay(100);
}
