#include <unity.h>
#include <string.h>
#include "ns_config_parse.h"

void setUp(void) {}
void tearDown(void) {}

/* ── Helper: make a mutable copy of a string literal ──────────── */

static char buf[4096];

static size_t loadBuf(const char *src) {
    size_t len = strlen(src);
    memcpy(buf, src, len + 1);
    return len;
}

/* ── configDefaults tests ─────────────────────────────────────── */

void test_defaults_device_name(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    TEST_ASSERT_EQUAL_STRING("t1display", cfg.deviceName);
}

void test_defaults_timezone(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    TEST_ASSERT_EQUAL_INT(3600, cfg.timeZone);
}

void test_defaults_thresholds(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.5f, cfg.yellow_low);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 9.0f, cfg.yellow_high);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.9f, cfg.red_low);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 11.0f, cfg.red_high);
}

void test_defaults_brightness(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    TEST_ASSERT_EQUAL_INT(10, cfg.brightness1);
    TEST_ASSERT_EQUAL_INT(50, cfg.brightness2);
    TEST_ASSERT_EQUAL_INT(100, cfg.brightness3);
}

void test_defaults_sound(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, cfg.snd_alarm);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.7f, cfg.snd_warning);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, cfg.snd_alarm_high);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 14.0f, cfg.snd_warning_high);
    TEST_ASSERT_EQUAL_INT(20, cfg.snd_no_readings);
}

void test_defaults_volumes(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    TEST_ASSERT_EQUAL_INT(30, cfg.warning_volume);
    TEST_ASSERT_EQUAL_INT(100, cfg.alarm_volume);
}

void test_defaults_zeroed_strings(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    TEST_ASSERT_EQUAL_STRING("", cfg.url);
    TEST_ASSERT_EQUAL_STRING("", cfg.token);
    TEST_ASSERT_EQUAL_STRING("", cfg.userName);
    TEST_ASSERT_EQUAL_STRING("", cfg.wlanssid[0]);
}

/* ── Basic parsing tests ──────────────────────────────────────── */

void test_parse_simple_config(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[config]\n"
        "nightscout = https://my.nightscout.site\n"
        "token = abc123\n"
        "name = TestUser\n"
    );
    int n = parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(3, n);
    TEST_ASSERT_EQUAL_STRING("https://my.nightscout.site", cfg.url);
    TEST_ASSERT_EQUAL_STRING("abc123", cfg.token);
    TEST_ASSERT_EQUAL_STRING("TestUser", cfg.userName);
}

void test_parse_numeric_fields(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[config]\n"
        "time_zone = 7200\n"
        "dst = 1\n"
        "show_mgdl = 1\n"
        "show_current_time = 1\n"
        "default_page = 1\n"
        "sgv_only = 1\n"
        "info_line = 0\n"
        "date_format = 2\n"
        "time_format = 1\n"
    );
    int n = parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(9, n);
    TEST_ASSERT_EQUAL_INT(7200, cfg.timeZone);
    TEST_ASSERT_EQUAL_INT(1, cfg.dst);
    TEST_ASSERT_EQUAL_INT(1, cfg.show_mgdl);
    TEST_ASSERT_EQUAL_INT(1, cfg.show_current_time);
    TEST_ASSERT_EQUAL_INT(1, cfg.default_page);
    TEST_ASSERT_EQUAL_INT(1, cfg.sgv_only);
    TEST_ASSERT_EQUAL_INT(0, cfg.info_line);
    TEST_ASSERT_EQUAL_INT(2, cfg.date_format);
    TEST_ASSERT_EQUAL_INT(1, cfg.time_format);
}

void test_parse_float_fields(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[config]\n"
        "yellow_low = 5.0\n"
        "yellow_high = 10.0\n"
        "red_low = 4.0\n"
        "red_high = 12.0\n"
        "snd_alarm = 2.5\n"
        "snd_warning = 3.5\n"
        "snd_alarm_high = 18.0\n"
        "snd_warning_high = 13.0\n"
    );
    int n = parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(8, n);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, cfg.yellow_low);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, cfg.yellow_high);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.0f, cfg.red_low);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.0f, cfg.red_high);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.5f, cfg.snd_alarm);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.5f, cfg.snd_warning);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 18.0f, cfg.snd_alarm_high);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 13.0f, cfg.snd_warning_high);
}

void test_parse_brightness_and_volume(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[config]\n"
        "brightness1 = 5\n"
        "brightness2 = 40\n"
        "brightness3 = 80\n"
        "warning_volume = 50\n"
        "alarm_volume = 80\n"
    );
    int n = parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(5, n);
    TEST_ASSERT_EQUAL_INT(5, cfg.brightness1);
    TEST_ASSERT_EQUAL_INT(40, cfg.brightness2);
    TEST_ASSERT_EQUAL_INT(80, cfg.brightness3);
    TEST_ASSERT_EQUAL_INT(50, cfg.warning_volume);
    TEST_ASSERT_EQUAL_INT(80, cfg.alarm_volume);
}

/* ── WiFi section parsing ─────────────────────────────────────── */

void test_parse_single_wlan(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[wlan1]\n"
        "ssid = MyNetwork\n"
        "pass = secret123\n"
    );
    int n = parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL_STRING("MyNetwork", cfg.wlanssid[0]);
    TEST_ASSERT_EQUAL_STRING("secret123", cfg.wlanpass[0]);
}

void test_parse_multiple_wlans(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[wlan1]\n"
        "ssid = Home\n"
        "pass = pass1\n"
        "[wlan2]\n"
        "ssid = Office\n"
        "pass = pass2\n"
        "[wlan3]\n"
        "ssid = Mobile\n"
        "pass = pass3\n"
    );
    int n = parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(6, n);
    TEST_ASSERT_EQUAL_STRING("Home", cfg.wlanssid[0]);
    TEST_ASSERT_EQUAL_STRING("pass1", cfg.wlanpass[0]);
    TEST_ASSERT_EQUAL_STRING("Office", cfg.wlanssid[1]);
    TEST_ASSERT_EQUAL_STRING("pass2", cfg.wlanpass[1]);
    TEST_ASSERT_EQUAL_STRING("Mobile", cfg.wlanssid[2]);
    TEST_ASSERT_EQUAL_STRING("pass3", cfg.wlanpass[2]);
}

void test_parse_wlan10_max_index(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[wlan10]\n"
        "ssid = LastOne\n"
        "pass = lastpass\n"
    );
    int n = parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL_STRING("LastOne", cfg.wlanssid[9]);
    TEST_ASSERT_EQUAL_STRING("lastpass", cfg.wlanpass[9]);
}

void test_parse_wlan_out_of_range_ignored(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[wlan11]\n"
        "ssid = TooMany\n"
        "pass = nope\n"
    );
    int n = parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(0, n);  // out of range, nothing parsed
}

/* ── Mixed config + wlan ──────────────────────────────────────── */

void test_parse_full_ini(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[config]\n"
        "nightscout = https://ns.example.com\n"
        "token = mytoken\n"
        "name = Eric\n"
        "device_name = BedsideMon\n"
        "time_zone = -18000\n"
        "show_mgdl = 1\n"
        "yellow_low = 4.0\n"
        "yellow_high = 10.0\n"
        "red_low = 3.5\n"
        "red_high = 13.9\n"
        "brightness1 = 5\n"
        "\n"
        "[wlan1]\n"
        "ssid = HomeWiFi\n"
        "pass = wifipass\n"
        "\n"
        "[wlan2]\n"
        "ssid = BackupNet\n"
        "pass = backup123\n"
    );
    int n = parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(15, n);
    TEST_ASSERT_EQUAL_STRING("https://ns.example.com", cfg.url);
    TEST_ASSERT_EQUAL_STRING("mytoken", cfg.token);
    TEST_ASSERT_EQUAL_STRING("Eric", cfg.userName);
    TEST_ASSERT_EQUAL_STRING("BedsideMon", cfg.deviceName);
    TEST_ASSERT_EQUAL_INT(-18000, cfg.timeZone);
    TEST_ASSERT_EQUAL_INT(1, cfg.show_mgdl);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.0f, cfg.yellow_low);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, cfg.yellow_high);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.5f, cfg.red_low);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 13.9f, cfg.red_high);
    TEST_ASSERT_EQUAL_INT(5, cfg.brightness1);
    TEST_ASSERT_EQUAL_STRING("HomeWiFi", cfg.wlanssid[0]);
    TEST_ASSERT_EQUAL_STRING("wifipass", cfg.wlanpass[0]);
    TEST_ASSERT_EQUAL_STRING("BackupNet", cfg.wlanssid[1]);
    TEST_ASSERT_EQUAL_STRING("backup123", cfg.wlanpass[1]);
}

/* ── Edge cases ───────────────────────────────────────────────── */

void test_parse_comments_and_blanks(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "; This is a comment\n"
        "# Another comment\n"
        "\n"
        "[config]\n"
        "nightscout = https://example.com\n"
        "; inline comment on next line\n"
        "token = abc\n"
    );
    int n = parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL_STRING("https://example.com", cfg.url);
    TEST_ASSERT_EQUAL_STRING("abc", cfg.token);
}

void test_parse_whitespace_around_equals(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[config]\n"
        "  nightscout  =  https://padded.example.com  \n"
        "  token=notrimmed\n"
    );
    int n = parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL_STRING("https://padded.example.com", cfg.url);
    TEST_ASSERT_EQUAL_STRING("notrimmed", cfg.token);
}

void test_parse_unknown_keys_ignored(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[config]\n"
        "nightscout = https://example.com\n"
        "bogus_key = whatever\n"
        "another_unknown = 42\n"
    );
    int n = parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(1, n);  // only nightscout counted
    TEST_ASSERT_EQUAL_STRING("https://example.com", cfg.url);
}

void test_parse_empty_buffer(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    buf[0] = '\0';
    int n = parseConfigBuffer(buf, 0, &cfg);
    TEST_ASSERT_EQUAL_INT(0, n);
}

void test_parse_no_section_header(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    // Keys before any section header — should be ignored since currentWlan starts at -1
    // which means [config] section, so they should still parse
    size_t len = loadBuf(
        "nightscout = https://nosection.example.com\n"
    );
    int n = parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(1, n);
    TEST_ASSERT_EQUAL_STRING("https://nosection.example.com", cfg.url);
}

void test_parse_restart_at_logged_errors(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[config]\n"
        "restart_at_logged_errors = 50\n"
        "snooze_timeout = 15\n"
        "alarm_repeat = 3\n"
        "snd_no_readings = 30\n"
    );
    int n = parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(4, n);
    TEST_ASSERT_EQUAL_INT(50, cfg.restart_at_logged_errors);
    TEST_ASSERT_EQUAL_INT(15, cfg.snooze_timeout);
    TEST_ASSERT_EQUAL_INT(3, cfg.alarm_repeat);
    TEST_ASSERT_EQUAL_INT(30, cfg.snd_no_readings);
}

void test_parse_carriage_return_line_endings(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[config]\r\n"
        "nightscout = https://crlf.example.com\r\n"
        "token = crlftoken\r\n"
    );
    int n = parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL_STRING("https://crlf.example.com", cfg.url);
    TEST_ASSERT_EQUAL_STRING("crlftoken", cfg.token);
}

void test_parse_section_switching(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[wlan1]\n"
        "ssid = First\n"
        "[config]\n"
        "nightscout = https://example.com\n"
        "[wlan2]\n"
        "ssid = Second\n"
    );
    int n = parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(3, n);
    TEST_ASSERT_EQUAL_STRING("First", cfg.wlanssid[0]);
    TEST_ASSERT_EQUAL_STRING("https://example.com", cfg.url);
    TEST_ASSERT_EQUAL_STRING("Second", cfg.wlanssid[1]);
}

/* ── Validation / clamping tests ──────────────────────────────── */

void test_validate_brightness_clamped(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[config]\n"
        "brightness1 = -10\n"
        "brightness2 = 200\n"
        "brightness3 = 50\n"
    );
    parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(0, cfg.brightness1);
    TEST_ASSERT_EQUAL_INT(100, cfg.brightness2);
    TEST_ASSERT_EQUAL_INT(50, cfg.brightness3);
}

void test_validate_boolean_fields_clamped(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[config]\n"
        "show_mgdl = 5\n"
        "sgv_only = -1\n"
        "info_line = 2\n"
    );
    parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(1, cfg.show_mgdl);
    TEST_ASSERT_EQUAL_INT(0, cfg.sgv_only);
    TEST_ASSERT_EQUAL_INT(1, cfg.info_line);
}

void test_validate_volume_clamped(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[config]\n"
        "warning_volume = 150\n"
        "alarm_volume = -20\n"
    );
    parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(100, cfg.warning_volume);
    TEST_ASSERT_EQUAL_INT(0, cfg.alarm_volume);
}

void test_validate_timeout_clamped(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[config]\n"
        "snooze_timeout = 9999\n"
        "alarm_repeat = -5\n"
        "snd_no_readings = 2000\n"
    );
    parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(1440, cfg.snooze_timeout);
    TEST_ASSERT_EQUAL_INT(0, cfg.alarm_repeat);
    TEST_ASSERT_EQUAL_INT(1440, cfg.snd_no_readings);
}

void test_snooze_timeout_cannot_be_zero(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf("[config]\nsnooze_timeout = 0\n");
    parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, cfg.snooze_timeout,
        "a zero snooze would make the snooze button silently do nothing");
    cfg.snooze_timeout = -3;
    validateConfig(&cfg);
    TEST_ASSERT_EQUAL_INT(1, cfg.snooze_timeout);
}

void test_validate_date_format_clamped(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t len = loadBuf(
        "[config]\n"
        "date_format = 10\n"
        "default_page = 5\n"
    );
    parseConfigBuffer(buf, len, &cfg);
    TEST_ASSERT_EQUAL_INT(3, cfg.date_format);
    TEST_ASSERT_EQUAL_INT(1, cfg.default_page);
}


/* ── Unknown key accounting ───────────────────────────────────── */

void test_defaults_unknown_keys_zero(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    TEST_ASSERT_EQUAL_INT(0, cfg.unknownKeys);
}

void test_known_keys_not_counted_unknown(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t n = loadBuf("[config]\nname = Eric\ntime_zone = -18000\n");
    parseConfigBuffer(buf, n, &cfg);
    TEST_ASSERT_EQUAL_INT(0, cfg.unknownKeys);
}

void test_unknown_key_counted(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t n = loadBuf("[config]\nname = Eric\nbootpic = /x.jpg\n");
    parseConfigBuffer(buf, n, &cfg);
    TEST_ASSERT_EQUAL_INT(1, cfg.unknownKeys);
}

void test_multiple_unknown_keys_counted(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t n = loadBuf("[config]\nbootpic = /x.jpg\ndisplay_rotation = 1\ntemperature_unit = 1\n");
    parseConfigBuffer(buf, n, &cfg);
    TEST_ASSERT_EQUAL_INT(3, cfg.unknownKeys);
}

void test_unknown_key_in_wlan_section_counted(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t n = loadBuf("[wlan1]\nssid = Net\npass = pw\nsecurity = wpa2\n");
    parseConfigBuffer(buf, n, &cfg);
    TEST_ASSERT_EQUAL_INT(1, cfg.unknownKeys);
}

void test_unknown_keys_do_not_affect_parsed_count(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t n = loadBuf("[config]\nname = Eric\nbootpic = /x.jpg\n");
    int parsed = parseConfigBuffer(buf, n, &cfg);
    TEST_ASSERT_EQUAL_INT(1, parsed);
    TEST_ASSERT_EQUAL_INT(1, cfg.unknownKeys);
}


/* ── Section isolation ────────────────────────────────────────── */

void test_out_of_range_section_does_not_hijack_previous(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t n = loadBuf("[wlan1]\nssid = HomeNet\npass = secret\n"
                       "[wlan42]\nssid = EvilAP\npass = pwned\n");
    parseConfigBuffer(buf, n, &cfg);
    TEST_ASSERT_EQUAL_STRING("HomeNet", cfg.wlanssid[0]);
    TEST_ASSERT_EQUAL_STRING("secret", cfg.wlanpass[0]);
}

void test_unknown_section_does_not_hijack_previous(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t n = loadBuf("[wlan1]\nssid = HomeNet\npass = secret\n"
                       "[bogus]\nssid = Hijack\npass = h2\n");
    parseConfigBuffer(buf, n, &cfg);
    TEST_ASSERT_EQUAL_STRING("HomeNet", cfg.wlanssid[0]);
    TEST_ASSERT_EQUAL_STRING("secret", cfg.wlanpass[0]);
}

void test_unterminated_section_does_not_hijack_previous(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t n = loadBuf("[wlan1]\nssid = HomeNet\n[wlan2\nssid = Broken\n");
    parseConfigBuffer(buf, n, &cfg);
    TEST_ASSERT_EQUAL_STRING("HomeNet", cfg.wlanssid[0]);
}

void test_config_section_recovers_after_bad_section(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t n = loadBuf("[bogus]\nname = Ignored\n[config]\nname = Eric\n");
    parseConfigBuffer(buf, n, &cfg);
    TEST_ASSERT_EQUAL_STRING("Eric", cfg.userName);
}

void test_keys_in_bad_section_are_not_counted_unknown(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t n = loadBuf("[bogus]\nwhatever = 1\nmore = 2\n");
    parseConfigBuffer(buf, n, &cfg);
    TEST_ASSERT_EQUAL_INT(0, cfg.unknownKeys);
}

/* ── Main ─────────────────────────────────────────────────────── */

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_defaults_unknown_keys_zero);
    RUN_TEST(test_known_keys_not_counted_unknown);
    RUN_TEST(test_unknown_key_counted);
    RUN_TEST(test_multiple_unknown_keys_counted);
    RUN_TEST(test_unknown_key_in_wlan_section_counted);
    RUN_TEST(test_unknown_keys_do_not_affect_parsed_count);

    RUN_TEST(test_out_of_range_section_does_not_hijack_previous);
    RUN_TEST(test_unknown_section_does_not_hijack_previous);
    RUN_TEST(test_unterminated_section_does_not_hijack_previous);
    RUN_TEST(test_config_section_recovers_after_bad_section);
    RUN_TEST(test_keys_in_bad_section_are_not_counted_unknown);

    /* Defaults */
    RUN_TEST(test_defaults_device_name);
    RUN_TEST(test_defaults_timezone);
    RUN_TEST(test_defaults_thresholds);
    RUN_TEST(test_defaults_brightness);
    RUN_TEST(test_defaults_sound);
    RUN_TEST(test_defaults_volumes);
    RUN_TEST(test_defaults_zeroed_strings);

    /* Basic parsing */
    RUN_TEST(test_parse_simple_config);
    RUN_TEST(test_parse_numeric_fields);
    RUN_TEST(test_parse_float_fields);
    RUN_TEST(test_parse_brightness_and_volume);

    /* WiFi sections */
    RUN_TEST(test_parse_single_wlan);
    RUN_TEST(test_parse_multiple_wlans);
    RUN_TEST(test_parse_wlan10_max_index);
    RUN_TEST(test_parse_wlan_out_of_range_ignored);

    /* Mixed */
    RUN_TEST(test_parse_full_ini);

    /* Edge cases */
    RUN_TEST(test_parse_comments_and_blanks);
    RUN_TEST(test_parse_whitespace_around_equals);
    RUN_TEST(test_parse_unknown_keys_ignored);
    RUN_TEST(test_parse_empty_buffer);
    RUN_TEST(test_parse_no_section_header);
    RUN_TEST(test_parse_restart_at_logged_errors);
    RUN_TEST(test_parse_carriage_return_line_endings);
    RUN_TEST(test_parse_section_switching);

    /* Validation / clamping */
    RUN_TEST(test_validate_brightness_clamped);
    RUN_TEST(test_validate_boolean_fields_clamped);
    RUN_TEST(test_validate_volume_clamped);
    RUN_TEST(test_validate_timeout_clamped);
    RUN_TEST(test_snooze_timeout_cannot_be_zero);
    RUN_TEST(test_validate_date_format_clamped);

    return UNITY_END();
}
