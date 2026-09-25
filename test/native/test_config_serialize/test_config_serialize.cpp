#include <unity.h>
#include <string.h>
#include "ns_config_parse.h"

void setUp(void) {}
void tearDown(void) {}

/* ── Round-trip: defaults → serialize → parse → compare ──────── */

void test_roundtrip_defaults(void) {
    ParsedConfig orig;
    configDefaults(&orig);

    char ini[4096];
    int wrote = serializeConfigINI(&orig, ini, sizeof(ini));
    TEST_ASSERT_GREATER_THAN(0, wrote);

    // Parse it back
    ParsedConfig parsed;
    configDefaults(&parsed);  // start from defaults (same as original flow)
    char iniBuf[4096];
    memcpy(iniBuf, ini, (size_t)wrote + 1);
    int n = parseConfigBuffer(iniBuf, (size_t)wrote, &parsed);
    TEST_ASSERT_GREATER_THAN(0, n);

    // Compare all fields
    TEST_ASSERT_EQUAL_STRING(orig.url, parsed.url);
    TEST_ASSERT_EQUAL_STRING(orig.token, parsed.token);
    TEST_ASSERT_EQUAL_STRING(orig.userName, parsed.userName);
    TEST_ASSERT_EQUAL_STRING(orig.deviceName, parsed.deviceName);
    TEST_ASSERT_EQUAL_INT(orig.timeZone, parsed.timeZone);
    TEST_ASSERT_EQUAL_INT(orig.dst, parsed.dst);
    TEST_ASSERT_EQUAL_INT(orig.show_mgdl, parsed.show_mgdl);
    TEST_ASSERT_EQUAL_INT(orig.show_current_time, parsed.show_current_time);
    TEST_ASSERT_EQUAL_INT(orig.default_page, parsed.default_page);
    TEST_ASSERT_EQUAL_INT(orig.sgv_only, parsed.sgv_only);
    TEST_ASSERT_EQUAL_INT(orig.info_line, parsed.info_line);
    TEST_ASSERT_EQUAL_INT(orig.date_format, parsed.date_format);
    TEST_ASSERT_EQUAL_INT(orig.time_format, parsed.time_format);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, orig.yellow_low, parsed.yellow_low);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, orig.yellow_high, parsed.yellow_high);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, orig.red_low, parsed.red_low);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, orig.red_high, parsed.red_high);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, orig.snd_alarm, parsed.snd_alarm);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, orig.snd_warning, parsed.snd_warning);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, orig.snd_alarm_high, parsed.snd_alarm_high);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, orig.snd_warning_high, parsed.snd_warning_high);
    TEST_ASSERT_EQUAL_INT(orig.snd_no_readings, parsed.snd_no_readings);
    TEST_ASSERT_EQUAL_INT(orig.snooze_timeout, parsed.snooze_timeout);
    TEST_ASSERT_EQUAL_INT(orig.alarm_repeat, parsed.alarm_repeat);
    TEST_ASSERT_EQUAL_INT(orig.warning_volume, parsed.warning_volume);
    TEST_ASSERT_EQUAL_INT(orig.alarm_volume, parsed.alarm_volume);
    TEST_ASSERT_EQUAL_INT(orig.brightness1, parsed.brightness1);
    TEST_ASSERT_EQUAL_INT(orig.brightness2, parsed.brightness2);
    TEST_ASSERT_EQUAL_INT(orig.brightness3, parsed.brightness3);
    TEST_ASSERT_EQUAL_INT(orig.restart_at_logged_errors, parsed.restart_at_logged_errors);
    TEST_ASSERT_EQUAL_STRING(orig.restart_at_time, parsed.restart_at_time);
    TEST_ASSERT_EQUAL_INT(orig.snd_loop_error, parsed.snd_loop_error);
}

/* ── Round-trip: custom values ───────────────────────────────── */

void test_roundtrip_custom_values(void) {
    ParsedConfig orig;
    configDefaults(&orig);

    strlcpy(orig.url, "https://my.nightscout.site", sizeof(orig.url));
    strlcpy(orig.token, "abc-123-token", sizeof(orig.token));
    strlcpy(orig.userName, "Eric", sizeof(orig.userName));
    strlcpy(orig.deviceName, "BedsideMon", sizeof(orig.deviceName));
    orig.timeZone = -18000;
    orig.dst = 1;
    strlcpy(orig.tz, "EST5EDT,M3.2.0,M11.1.0", sizeof(orig.tz));
    orig.show_mgdl = 1;
    orig.show_current_time = 1;
    orig.default_page = 1;
    orig.sgv_only = 1;
    orig.info_line = 0;
    orig.date_format = 1;
    orig.time_format = 1;
    orig.yellow_low = 5.0f;
    orig.yellow_high = 10.0f;
    orig.red_low = 4.0f;
    orig.red_high = 12.0f;
    orig.snd_alarm = 2.5f;
    orig.snd_warning = 3.5f;
    orig.snd_alarm_high = 18.0f;
    orig.snd_warning_high = 13.0f;
    orig.snd_no_readings = 30;
    orig.snooze_timeout = 15;
    orig.alarm_repeat = 3;
    orig.warning_volume = 50;
    orig.alarm_volume = 80;
    orig.brightness1 = 5;
    orig.brightness2 = 40;
    orig.brightness3 = 80;
    orig.restart_at_logged_errors = 50;
    strlcpy(orig.restart_at_time, "03:00", sizeof(orig.restart_at_time));
    orig.snd_loop_error = 0;

    strlcpy(orig.wlanssid[0], "HomeWiFi", 64);
    strlcpy(orig.wlanpass[0], "homepass", 64);
    strlcpy(orig.wlanssid[1], "OfficeNet", 64);
    strlcpy(orig.wlanpass[1], "officepass", 64);

    char ini[4096];
    int wrote = serializeConfigINI(&orig, ini, sizeof(ini));
    TEST_ASSERT_GREATER_THAN(0, wrote);

    ParsedConfig parsed;
    configDefaults(&parsed);
    char iniBuf[4096];
    memcpy(iniBuf, ini, (size_t)wrote + 1);
    int n = parseConfigBuffer(iniBuf, (size_t)wrote, &parsed);
    TEST_ASSERT_GREATER_THAN(0, n);

    TEST_ASSERT_EQUAL_STRING("https://my.nightscout.site", parsed.url);
    TEST_ASSERT_EQUAL_STRING("abc-123-token", parsed.token);
    TEST_ASSERT_EQUAL_STRING("Eric", parsed.userName);
    TEST_ASSERT_EQUAL_STRING("BedsideMon", parsed.deviceName);
    TEST_ASSERT_EQUAL_INT(-18000, parsed.timeZone);
    TEST_ASSERT_EQUAL_INT(1, parsed.dst);
    TEST_ASSERT_EQUAL_STRING("EST5EDT,M3.2.0,M11.1.0", parsed.tz);
    TEST_ASSERT_EQUAL_INT(1, parsed.show_mgdl);
    TEST_ASSERT_EQUAL_INT(1, parsed.default_page);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 5.0f, parsed.yellow_low);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 12.0f, parsed.red_high);
    TEST_ASSERT_EQUAL_INT(50, parsed.warning_volume);
    TEST_ASSERT_EQUAL_INT(5, parsed.brightness1);
    TEST_ASSERT_EQUAL_STRING("03:00", parsed.restart_at_time);
    TEST_ASSERT_EQUAL_INT(0, parsed.snd_loop_error);
    TEST_ASSERT_EQUAL_STRING("HomeWiFi", parsed.wlanssid[0]);
    TEST_ASSERT_EQUAL_STRING("homepass", parsed.wlanpass[0]);
    TEST_ASSERT_EQUAL_STRING("OfficeNet", parsed.wlanssid[1]);
    TEST_ASSERT_EQUAL_STRING("officepass", parsed.wlanpass[1]);
    TEST_ASSERT_EQUAL_STRING("", parsed.wlanssid[2]);
}

/* ── Buffer too small ────────────────────────────────────────── */

void test_serialize_buffer_too_small(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);

    char tiny[10];
    int wrote = serializeConfigINI(&cfg, tiny, sizeof(tiny));
    TEST_ASSERT_EQUAL_INT(0, wrote);
}

/* ── Only non-empty WiFi slots are written ───────────────────── */

void test_serialize_skips_empty_wifi(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);

    // Only set wlan2 (index 1), leave others empty
    strlcpy(cfg.wlanssid[1], "OnlyThis", 64);
    strlcpy(cfg.wlanpass[1], "secret", 64);

    char ini[4096];
    int wrote = serializeConfigINI(&cfg, ini, sizeof(ini));
    TEST_ASSERT_GREATER_THAN(0, wrote);

    // Should contain [wlan2] but not [wlan1]
    TEST_ASSERT_NOT_NULL(strstr(ini, "[wlan2]"));
    TEST_ASSERT_NULL(strstr(ini, "[wlan1]"));
    TEST_ASSERT_NULL(strstr(ini, "[wlan3]"));
}

/* ── Null/invalid args ───────────────────────────────────────── */

void test_serialize_null_args(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    char buf[256];

    TEST_ASSERT_EQUAL_INT(0, serializeConfigINI(NULL, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(0, serializeConfigINI(&cfg, NULL, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(0, serializeConfigINI(&cfg, buf, 0));
}

/* ── Main ─────────────────────────────────────────────────────── */

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_roundtrip_defaults);
    RUN_TEST(test_roundtrip_custom_values);
    RUN_TEST(test_serialize_buffer_too_small);
    RUN_TEST(test_serialize_skips_empty_wifi);
    RUN_TEST(test_serialize_null_args);

    return UNITY_END();
}
