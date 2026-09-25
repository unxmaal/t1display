# Rule 06: Networking

## WiFi

STA mode only, via `WiFiMulti` with up to `CFG_MAX_WLAN` configured networks.
There is no AP mode, no captive portal and no provisioning flow — credentials
come from `/M5NS.INI` on the SD card.

`Wokwi-GUEST` is joined only under `-DWOKWI_SIM`. Never let an open network into
a release build.

## Nightscout requests

URL assembly lives in `lib/ns_url_build/` so the request shape is testable.

1. **SGV:** `GET <base>/api/v1/entries.json?count=1&find[type][$eq]=sgv`
2. **Delta:** `GET <base>/api/v2/properties/delta`

`count=1` because only the newest sgv record is used. The type filter is always
sent — without it, `count=1` could return an `mbg` or `cal` record.

Redirects are followed with `HTTPC_STRICT_FOLLOW_REDIRECTS`. Do not use
`FORCE`, which has a known CA-bundle bug on cross-host redirects.

TLS currently uses `setInsecure()`, so certificates are not validated. That is a
known gap, not a decision to copy elsewhere.

## Threading

Fetches run on a FreeRTOS task pinned to core 0. `NSinfo` and the error log are
exchanged with the render loop under `nsMutex`, with the task operating on a
scratch copy so a slow request never holds the lock.

Never call `readNightscout()` from `loop()`. Blocking the loop stops touch
input, the snooze button and alarm checks.

## Web config

`WebServer` on port 80, behind HTTP Basic auth when `web_user` and `web_pass`
are both set. `/test` is POST-only — a GET with side effects is reachable from
any page via an `<img>` tag.

## OTA

ArduinoOTA, started only when `ota_password` is set. With it unset the port
stays closed rather than open and unauthenticated.

mDNS is started by the firmware itself once Wi-Fi is up, advertising `http` on
port 80, so `<device_name>.local` resolves whether or not OTA is enabled.
ArduinoOTA's own mDNS is disabled; `setupOTA()` adds the `arduino` service.

The web config, OTA and mDNS start the first time `loop()` sees Wi-Fi connected
(`servicesDue()` in `lib/ns_runtime/`), not only at boot. SNTP is configured
before connecting, so the clock also syncs after a late connect.