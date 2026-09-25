#include <unity.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "ns_config_parse.h"
#include "ns_pure_logic.h"
#include "ns_restart_schedule.h"

void setUp(void) {}
void tearDown(void) {}

static ParsedConfig cfg;

static void parse(const char *ini) {
    configDefaults(&cfg);
    parseConfigBuffer(ini, strlen(ini), &cfg);
}

void test_clean_ini_has_no_errors(void) {
    parse("[config]\nnightscout = https://ns.example.com\nred_high = 11.0\n"
          "restart_at_time = 04:30\n[wlan1]\nssid = home\npass = pw\n");
    TEST_ASSERT_EQUAL_INT(0, cfg.configErrors);
    char msg[48];
    formatConfigErrors(&cfg, msg, sizeof(msg));
    TEST_ASSERT_EQUAL_STRING("", msg);
}

void test_section_names_ignore_case(void) {
    parse("[Config]\nname = Eric\n[WLAN2]\nssid = cabin\n");
    TEST_ASSERT_EQUAL_STRING("Eric", cfg.userName);
    TEST_ASSERT_EQUAL_STRING("cabin", cfg.wlanssid[1]);
    TEST_ASSERT_EQUAL_INT(0, cfg.configErrors);
}

void test_unknown_section_is_an_error(void) {
    parse("[confg]\nname = Eric\n");
    TEST_ASSERT_EQUAL_INT(1, cfg.configErrors);
    TEST_ASSERT_EQUAL_STRING("[confg]", cfg.firstBadKey);
}

void test_unclosed_section_is_an_error(void) {
    parse("[wlan1\nssid = home\n");
    TEST_ASSERT_EQUAL_INT(1, cfg.configErrors);
    TEST_ASSERT_EQUAL_STRING("[wlan1", cfg.firstBadKey);
}

void test_unknown_key_is_an_error(void) {
    parse("[config]\nnighscout = https://x\n");
    TEST_ASSERT_EQUAL_INT(1, cfg.configErrors);
    TEST_ASSERT_EQUAL_STRING("nighscout", cfg.firstBadKey);
}

void test_unknown_wlan_key_is_an_error(void) {
    parse("[wlan1]\npassword = pw\n");
    TEST_ASSERT_EQUAL_INT(1, cfg.configErrors);
    TEST_ASSERT_EQUAL_STRING("password", cfg.firstBadKey);
}

void test_mgdl_threshold_in_ini_is_converted(void) {
    parse("[config]\nshow_mgdl = 1\nred_high = 180\n");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(180.0f / MGDL_PER_MMOL, cfg.red_high,
        "a threshold above 40 can only be mg/dL");
    TEST_ASSERT_EQUAL_INT(0, cfg.configErrors);
}

void test_unparsable_threshold_is_an_error(void) {
    parse("[config]\nred_high = abc\n");
    TEST_ASSERT_EQUAL_FLOAT(11.0f, cfg.red_high);
    TEST_ASSERT_EQUAL_INT(1, cfg.configErrors);
    TEST_ASSERT_EQUAL_STRING("red_high", cfg.firstBadKey);
}

void test_out_of_range_int_is_clamped_and_reported(void) {
    parse("[config]\nbrightness1 = 150\n");
    TEST_ASSERT_EQUAL_INT(100, cfg.brightness1);
    TEST_ASSERT_EQUAL_INT(1, cfg.configErrors);
    TEST_ASSERT_EQUAL_STRING("brightness1", cfg.firstBadKey);
}

void test_unparsable_int_is_an_error(void) {
    parse("[config]\nalarm_volume = loud\n");
    TEST_ASSERT_EQUAL_INT(1, cfg.configErrors);
}

void test_truncated_value_is_an_error(void) {
    parse("[config]\ndevice_name = this-device-name-is-far-too-long-to-fit\n");
    TEST_ASSERT_EQUAL_INT(1, cfg.configErrors);
    TEST_ASSERT_EQUAL_STRING("device_name", cfg.firstBadKey);
}

void test_truncated_wlan_value_is_an_error(void) {
    parse("[wlan1]\nssid = 0123456789012345678901234567890123456789012345678901234567890123456789\n");
    TEST_ASSERT_EQUAL_INT(1, cfg.configErrors);
    TEST_ASSERT_EQUAL_STRING("ssid", cfg.firstBadKey);
}

void test_overlong_line_is_an_error(void) {
    char ini[400] = "[config]\nnightscout = https://";
    size_t n = strlen(ini);
    memset(ini + n, 'a', 300);
    strcpy(ini + n + 300, "\n");
    parse(ini);
    TEST_ASSERT_TRUE(cfg.configErrors >= 1);
    TEST_ASSERT_EQUAL_STRING("nightscout", cfg.firstBadKey);
}

void test_overlong_line_without_a_key_is_an_error(void) {
    char ini[400] = "[config]\n";
    size_t n = strlen(ini);
    memset(ini + n, 'a', 300);
    strcpy(ini + n + 300, "\n");
    parse(ini);
    TEST_ASSERT_EQUAL_INT(1, cfg.configErrors);
    TEST_ASSERT_EQUAL_STRING("long line", cfg.firstBadKey);
}

void test_only_first_bad_key_is_named(void) {
    parse("[config]\nfoo = 1\nbar = 2\n");
    TEST_ASSERT_EQUAL_INT(2, cfg.configErrors);
    TEST_ASSERT_EQUAL_STRING("foo", cfg.firstBadKey);
}

void test_malformed_restart_time_reverts_to_nores(void) {
    const char *bad[] = {"25:00", "0400", "04:60", "noon", "04:30pm"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        char ini[64];
        snprintf(ini, sizeof(ini), "[config]\nrestart_at_time = %s\n", bad[i]);
        parse(ini);
        TEST_ASSERT_EQUAL_STRING("NORES", cfg.restart_at_time);
        TEST_ASSERT_EQUAL_INT(1, cfg.configErrors);
        TEST_ASSERT_EQUAL_STRING("restart_at_time", cfg.firstBadKey);
    }
}

void test_valid_restart_times_are_kept(void) {
    parse("[config]\nrestart_at_time = 04:30\n");
    TEST_ASSERT_EQUAL_STRING("04:30", cfg.restart_at_time);
    parse("[config]\nrestart_at_time = 7:5\n");
    TEST_ASSERT_EQUAL_STRING("7:5", cfg.restart_at_time);
    parse("[config]\nrestart_at_time = NORES\n");
    TEST_ASSERT_EQUAL_INT(0, cfg.configErrors);
    parse("[config]\nrestart_at_time = \n");
    TEST_ASSERT_EQUAL_STRING("NORES", cfg.restart_at_time);
    TEST_ASSERT_EQUAL_INT(0, cfg.configErrors);
}

void test_restart_time_predicate(void) {
    TEST_ASSERT_TRUE(restartTimeValid("00:00"));
    TEST_ASSERT_TRUE(restartTimeValid("23:59"));
    TEST_ASSERT_FALSE(restartTimeValid("24:00"));
    TEST_ASSERT_FALSE(restartTimeValid(""));
    TEST_ASSERT_FALSE(restartTimeValid(NULL));
    TEST_ASSERT_FALSE(restartTimeValid("NORES"));
}

void test_inverted_alarm_thresholds_revert_to_defaults(void) {
    parse("[config]\nsnd_warning = 25\n");
    TEST_ASSERT_EQUAL_FLOAT(3.7f, cfg.snd_warning);
    TEST_ASSERT_EQUAL_INT(1, cfg.configErrors);
    TEST_ASSERT_EQUAL_STRING("thresholds", cfg.firstBadKey);
    int level = alarmLevel(22.0f, cfg.snd_alarm, cfg.snd_warning,
                           cfg.snd_alarm_high, cfg.snd_warning_high, 0, 20, false);
    TEST_ASSERT_EQUAL_INT_MESSAGE(ALARM_LEVEL_HIGH_ALARM, level,
        "a hyper emergency must never be classified as a low warning");
}

void test_whole_threshold_set_reverts_together(void) {
    parse("[config]\nsnd_alarm = 3.3\nsnd_alarm_high = 12\nsnd_warning_high = 15\n");
    TEST_ASSERT_EQUAL_FLOAT(3.0f, cfg.snd_alarm);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, cfg.snd_alarm_high);
    TEST_ASSERT_EQUAL_FLOAT(14.0f, cfg.snd_warning_high);
}

void test_warning_above_high_warning_reverts(void) {
    parse("[config]\nsnd_warning = 9\nsnd_warning_high = 8\n");
    TEST_ASSERT_EQUAL_FLOAT(3.7f, cfg.snd_warning);
    TEST_ASSERT_EQUAL_STRING("thresholds", cfg.firstBadKey);
}

void test_inverted_colour_bands_revert_to_defaults(void) {
    parse("[config]\nyellow_low = 12\n");
    TEST_ASSERT_EQUAL_FLOAT(4.5f, cfg.yellow_low);
    TEST_ASSERT_EQUAL_FLOAT(11.0f, cfg.red_high);
    TEST_ASSERT_EQUAL_INT(1, cfg.configErrors);
}

void test_red_inside_yellow_reverts(void) {
    parse("[config]\nred_low = 5.0\n");
    TEST_ASSERT_EQUAL_FLOAT(3.9f, cfg.red_low);
    parse("[config]\nred_high = 8.0\n");
    TEST_ASSERT_EQUAL_FLOAT(11.0f, cfg.red_high);
}

void test_equal_warning_and_alarm_are_allowed(void) {
    parse("[config]\nsnd_warning = 3.0\n");
    TEST_ASSERT_EQUAL_INT(0, cfg.configErrors);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, cfg.snd_warning);
}

void test_validate_repairs_non_finite_struct_values(void) {
    configDefaults(&cfg);
    cfg.snd_alarm = NAN;
    cfg.red_high  = 99.0f;
    validateConfig(&cfg);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, cfg.snd_alarm);
    TEST_ASSERT_EQUAL_FLOAT(11.0f, cfg.red_high);
}

void test_form_errors_are_counted(void) {
    configDefaults(&cfg);
    const ConfigKV kv[] = {{"restart_at_time", "25:00"}};
    applyConfigForm(&cfg, kv, 1);
    TEST_ASSERT_EQUAL_INT(1, cfg.configErrors);
    TEST_ASSERT_EQUAL_STRING("NORES", cfg.restart_at_time);
}

void test_form_save_replaces_earlier_errors(void) {
    parse("[config]\nfoo = 1\n");
    const ConfigKV kv[] = {{"name", "Eric"}};
    applyConfigForm(&cfg, kv, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, cfg.configErrors,
        "a clean save must clear the errors from the file it replaced");
    TEST_ASSERT_EQUAL_STRING("", cfg.firstBadKey);
}

void test_format_one_error(void) {
    parse("[config]\nred_high = abc\n");
    char msg[48];
    formatConfigErrors(&cfg, msg, sizeof(msg));
    TEST_ASSERT_EQUAL_STRING("1 config error: red_high", msg);
}

void test_format_several_errors(void) {
    parse("[config]\nfoo = 1\nbar = 2\nbaz = 3\n");
    char msg[48];
    formatConfigErrors(&cfg, msg, sizeof(msg));
    TEST_ASSERT_EQUAL_STRING("3 config errors: foo", msg);
}

void test_defaults_clear_errors(void) {
    parse("[config]\nfoo = 1\n");
    configDefaults(&cfg);
    TEST_ASSERT_EQUAL_INT(0, cfg.configErrors);
    TEST_ASSERT_EQUAL_STRING("", cfg.firstBadKey);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_clean_ini_has_no_errors);
    RUN_TEST(test_section_names_ignore_case);
    RUN_TEST(test_unknown_section_is_an_error);
    RUN_TEST(test_unclosed_section_is_an_error);
    RUN_TEST(test_unknown_key_is_an_error);
    RUN_TEST(test_unknown_wlan_key_is_an_error);
    RUN_TEST(test_mgdl_threshold_in_ini_is_converted);
    RUN_TEST(test_unparsable_threshold_is_an_error);
    RUN_TEST(test_out_of_range_int_is_clamped_and_reported);
    RUN_TEST(test_unparsable_int_is_an_error);
    RUN_TEST(test_truncated_value_is_an_error);
    RUN_TEST(test_truncated_wlan_value_is_an_error);
    RUN_TEST(test_overlong_line_is_an_error);
    RUN_TEST(test_overlong_line_without_a_key_is_an_error);
    RUN_TEST(test_only_first_bad_key_is_named);
    RUN_TEST(test_malformed_restart_time_reverts_to_nores);
    RUN_TEST(test_valid_restart_times_are_kept);
    RUN_TEST(test_restart_time_predicate);
    RUN_TEST(test_inverted_alarm_thresholds_revert_to_defaults);
    RUN_TEST(test_whole_threshold_set_reverts_together);
    RUN_TEST(test_warning_above_high_warning_reverts);
    RUN_TEST(test_inverted_colour_bands_revert_to_defaults);
    RUN_TEST(test_red_inside_yellow_reverts);
    RUN_TEST(test_equal_warning_and_alarm_are_allowed);
    RUN_TEST(test_validate_repairs_non_finite_struct_values);
    RUN_TEST(test_form_errors_are_counted);
    RUN_TEST(test_form_save_replaces_earlier_errors);
    RUN_TEST(test_format_one_error);
    RUN_TEST(test_format_several_errors);
    RUN_TEST(test_defaults_clear_errors);
    return UNITY_END();
}
