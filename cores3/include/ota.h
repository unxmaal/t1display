/*  ota.h
 *
 *  Copyright (C) 2024-2026 Eric Dodd <eric.e.dodd@gmail.com>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CORES3_OTA_H
#define CORES3_OTA_H

void setupOTA(const char* hostname, const char* password);

void handleOTA();

#endif
