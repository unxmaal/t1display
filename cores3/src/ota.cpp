/*  ota.cpp
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ota.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <M5Unified.h>

void setupOTA(const char* hostname, const char* password) {
    if (!password || password[0] == '\0') {
        Serial.println("[OTA] Disabled: no ota_password configured");
        return;
    }

    ArduinoOTA.setHostname(hostname);
    ArduinoOTA.setPassword(password);

    ArduinoOTA.onStart([]() {
        Serial.println("[OTA] Update starting...");
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("[OTA] Update complete, rebooting");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        if (total) Serial.printf("[OTA] Progress: %u%%\r", (progress * 100) / total);
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)         Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)   Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)     Serial.println("End Failed");
    });

    ArduinoOTA.setMdnsEnabled(false);
    ArduinoOTA.begin();
    MDNS.enableArduino(3232, true);
    Serial.printf("[OTA] Ready at %s.local\n", hostname);
}

void handleOTA() {
    ArduinoOTA.handle();
}
