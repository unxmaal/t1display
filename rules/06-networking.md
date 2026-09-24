# Rule 06: Networking

## WiFi

- **Station mode** (`WIFI_STA`): normal operation, `WiFiMulti` connects to strongest of up to 10 configured networks
- **AP mode** (`WIFI_AP`): bootstrap — device becomes access point with mDNS + captive portal DNS

## Nightscout API

Two sequential HTTP calls per update cycle (every 15 seconds when data is stale):

1. **SGV entries:** `GET https://<host>/api/v1/entries.json?count=1&find[type][$eq]=sgv`
   - With token: appended as `&token=<token>`
   - Optional SGV filter: `find[type][$eq]=sgv`
2. **Properties:** `GET https://<host>/api/v2/properties/iob,cob,delta,loop,basal`


## HTTPS

- Heroku URLs (`herokuapp.com`): use `WiFiClientSecure` with embedded Starfield Services Root CA cert
- Other HTTPS URLs: use plain `HTTPClient.begin(url)` — no cert verification
- HTTP 301/302 redirects are followed automatically

## JSON Parsing

- Single global `DynamicJsonDocument` (16KB) reused for all API calls
- `ARDUINOJSON_USE_LONG_LONG 1` required (Nightscout timestamps are ms since epoch)
- Pre-processing before parse: strip control chars <32, fix Unicode escapes, trim oversized date fields

## Web Server (Internal)

- Port 80, `WebServer` class
- Routes: `/` (config UI), `/update` (OTA), `/savecfg`, `/switch`, `/edititem`, `/getedititem`, `/clearconfigflash`
- mDNS: `<deviceName>.local`
- Disable via `cfg.disable_web_server = 1`

## UDP Snooze Sync

- Port 50555, subnet broadcast
- Protocol: `"M5_Nightscout SNOOZE: USR=<CRC16 of URL>, SnoozeUntil=<epoch>"`
- CRC16 of Nightscout URL used as namespace (avoids cross-household conflicts)
- All M5Stacks watching the same Nightscout instance honor the broadcast snooze
