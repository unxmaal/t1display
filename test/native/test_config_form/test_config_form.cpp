#include <unity.h>
#include <string.h>
#include "ns_config_parse.h"

void setUp(void) {}
void tearDown(void) {}

static ParsedConfig cfg;

static void applyForm(const ConfigKV *kv, int n) {
    applyConfigForm(&cfg, kv, n);
}

void test_mmol_values_stored_unchanged(void) {
    configDefaults(&cfg);
    const ConfigKV kv[] = {{"snd_alarm", "3.2"}, {"red_low", "4.0"}};
    applyForm(kv, 2);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.2f, cfg.snd_alarm);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.0f, cfg.red_low);
}

void test_mgdl_values_converted_to_mmol(void) {
    configDefaults(&cfg);
    cfg.show_mgdl = 1;
    const ConfigKV kv[] = {{"snd_alarm", "54"}};
    applyForm(kv, 1);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 3.0f, cfg.snd_alarm);
}

void test_switching_to_mgdl_preserves_thresholds(void) {
    configDefaults(&cfg);
    cfg.show_mgdl = 0;
    cfg.snd_alarm = 3.0f;
    const ConfigKV kv[] = {{"show_mgdl", "1"}, {"snd_alarm", "3.0"}};
    applyForm(kv, 2);
    TEST_ASSERT_EQUAL_INT(1, cfg.show_mgdl);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 3.0f, cfg.snd_alarm,
        "switching display units must not rescale the stored threshold");
}

void test_switching_to_mgdl_key_order_independent(void) {
    configDefaults(&cfg);
    cfg.show_mgdl = 0;
    const ConfigKV kv[] = {{"snd_alarm", "3.0"}, {"show_mgdl", "1"}};
    applyForm(kv, 2);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, cfg.snd_alarm);
}

void test_switching_to_mmol_preserves_thresholds(void) {
    configDefaults(&cfg);
    cfg.show_mgdl = 1;
    cfg.snd_alarm = 3.0f;
    const ConfigKV kv[] = {{"show_mgdl", "0"}, {"snd_alarm", "54"}};
    applyForm(kv, 2);
    TEST_ASSERT_EQUAL_INT(0, cfg.show_mgdl);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 3.0f, cfg.snd_alarm);
}

void test_switch_units_then_hypo_still_alarms(void) {
    configDefaults(&cfg);
    cfg.show_mgdl = 0;
    const ConfigKV kv[] = {{"show_mgdl", "1"}, {"snd_alarm", "3.0"}, {"snd_warning", "3.7"}};
    applyForm(kv, 3);
    TEST_ASSERT_TRUE_MESSAGE(2.5f <= cfg.snd_alarm,
        "a 2.5 mmol/L hypo must still be at or below the low-alarm threshold");
}

void test_form_applies_validation(void) {
    configDefaults(&cfg);
    const ConfigKV kv[] = {{"alarm_volume", "999"}, {"snooze_timeout", "999999"}};
    applyForm(kv, 2);
    TEST_ASSERT_TRUE(cfg.alarm_volume >= 0 && cfg.alarm_volume <= 100);
    TEST_ASSERT_TRUE(cfg.snooze_timeout >= 0 && cfg.snooze_timeout <= 1440);
}

void test_form_rejects_nan_threshold(void) {
    configDefaults(&cfg);
    const ConfigKV kv[] = {{"snd_alarm", "nan"}};
    applyForm(kv, 1);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, cfg.snd_alarm);
}

void test_form_sets_strings(void) {
    configDefaults(&cfg);
    const ConfigKV kv[] = {{"nightscout", "https://x.example.com"}, {"name", "Eric"}};
    applyForm(kv, 2);
    TEST_ASSERT_EQUAL_STRING("https://x.example.com", cfg.url);
    TEST_ASSERT_EQUAL_STRING("Eric", cfg.userName);
}

void test_form_sets_all_ten_wlan_slots(void) {
    configDefaults(&cfg);
    const ConfigKV kv[] = {
        {"wlan_ssid_1", "Net1"},  {"wlan_pass_1", "p1"},
        {"wlan_ssid_9", "Net9"},  {"wlan_pass_9", "p9"},
        {"wlan_ssid_10", "Net10"},{"wlan_pass_10", "p10"},
    };
    applyForm(kv, 6);
    TEST_ASSERT_EQUAL_STRING("Net1", cfg.wlanssid[0]);
    TEST_ASSERT_EQUAL_STRING("Net9", cfg.wlanssid[8]);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Net10", cfg.wlanssid[9],
        "all CFG_MAX_WLAN slots must be settable from the form");
    TEST_ASSERT_EQUAL_STRING("p10", cfg.wlanpass[9]);
}

void test_form_ignores_out_of_range_wlan_slot(void) {
    configDefaults(&cfg);
    const ConfigKV kv[] = {{"wlan_ssid_1", "Good"}, {"wlan_ssid_42", "Bad"}};
    applyForm(kv, 2);
    TEST_ASSERT_EQUAL_STRING("Good", cfg.wlanssid[0]);
}

void test_form_ignores_unknown_key(void) {
    configDefaults(&cfg);
    const ConfigKV kv[] = {{"bootpic", "/x.jpg"}, {"name", "Eric"}};
    applyForm(kv, 2);
    TEST_ASSERT_EQUAL_STRING("Eric", cfg.userName);
}

void test_form_result_roundtrips_through_ini(void) {
    configDefaults(&cfg);
    const ConfigKV kv[] = {{"show_mgdl", "1"}, {"snd_alarm", "3.0"}, {"device_name", "t1display"}};
    applyForm(kv, 3);

    char ini[4096];
    int n = serializeConfigINI(&cfg, ini, sizeof(ini));
    TEST_ASSERT_GREATER_THAN_INT(0, n);

    ParsedConfig again;
    configDefaults(&again);
    parseConfigBuffer(ini, (size_t)n, &again);
    TEST_ASSERT_EQUAL_INT(0, again.unknownKeys);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, cfg.snd_alarm, again.snd_alarm);
}


void test_security_keys_parse_and_roundtrip(void) {
    configDefaults(&cfg);
    const ConfigKV kv[] = {
        {"ota_password", "otasecret"},
        {"web_user", "edodd"},
        {"web_pass", "webs3cret"},
    };
    applyForm(kv, 3);
    TEST_ASSERT_EQUAL_STRING("otasecret", cfg.otaPassword);
    TEST_ASSERT_EQUAL_STRING("edodd", cfg.webUser);
    TEST_ASSERT_EQUAL_STRING("webs3cret", cfg.webPass);

    char ini[4096];
    int n = serializeConfigINI(&cfg, ini, sizeof(ini));
    TEST_ASSERT_GREATER_THAN_INT(0, n);

    ParsedConfig again;
    configDefaults(&again);
    parseConfigBuffer(ini, (size_t)n, &again);
    TEST_ASSERT_EQUAL_INT(0, again.unknownKeys);
    TEST_ASSERT_EQUAL_STRING("otasecret", again.otaPassword);
    TEST_ASSERT_EQUAL_STRING("edodd", again.webUser);
    TEST_ASSERT_EQUAL_STRING("webs3cret", again.webPass);
}

void test_security_keys_default_to_empty(void) {
    configDefaults(&cfg);
    TEST_ASSERT_EQUAL_STRING("", cfg.otaPassword);
    TEST_ASSERT_EQUAL_STRING("", cfg.webUser);
    TEST_ASSERT_EQUAL_STRING("", cfg.webPass);
}

void test_ota_disabled_without_password(void) {
    configDefaults(&cfg);
    TEST_ASSERT_FALSE_MESSAGE(configOtaEnabled(&cfg),
        "OTA must not open a port when no password is configured");
}

void test_ota_enabled_with_password(void) {
    configDefaults(&cfg);
    const ConfigKV kv[] = {{"ota_password", "s3cret"}};
    applyForm(kv, 1);
    TEST_ASSERT_TRUE(configOtaEnabled(&cfg));
}

void test_web_auth_required_only_when_both_set(void) {
    configDefaults(&cfg);
    TEST_ASSERT_FALSE(configWebAuthEnabled(&cfg));

    const ConfigKV userOnly[] = {{"web_user", "edodd"}};
    applyForm(userOnly, 1);
    TEST_ASSERT_FALSE(configWebAuthEnabled(&cfg));

    const ConfigKV both[] = {{"web_pass", "pw"}};
    applyForm(both, 1);
    TEST_ASSERT_TRUE(configWebAuthEnabled(&cfg));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_mmol_values_stored_unchanged);
    RUN_TEST(test_mgdl_values_converted_to_mmol);
    RUN_TEST(test_switching_to_mgdl_preserves_thresholds);
    RUN_TEST(test_switching_to_mgdl_key_order_independent);
    RUN_TEST(test_switching_to_mmol_preserves_thresholds);
    RUN_TEST(test_switch_units_then_hypo_still_alarms);
    RUN_TEST(test_form_applies_validation);
    RUN_TEST(test_form_rejects_nan_threshold);
    RUN_TEST(test_form_sets_strings);
    RUN_TEST(test_form_sets_all_ten_wlan_slots);
    RUN_TEST(test_form_ignores_out_of_range_wlan_slot);
    RUN_TEST(test_form_ignores_unknown_key);
    RUN_TEST(test_form_result_roundtrips_through_ini);
    RUN_TEST(test_security_keys_parse_and_roundtrip);
    RUN_TEST(test_security_keys_default_to_empty);
    RUN_TEST(test_ota_disabled_without_password);
    RUN_TEST(test_ota_enabled_with_password);
    RUN_TEST(test_web_auth_required_only_when_both_set);
    return UNITY_END();
}
