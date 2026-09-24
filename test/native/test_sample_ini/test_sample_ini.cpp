#include <unity.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ns_config_parse.h"
#include "ns_pure_logic.h"

void setUp(void) {}
void tearDown(void) {}

static char iniBuf[8192];
static ParsedConfig cfg;

static const char *candidatePaths[] = {
    "SD/M5NS.INI",
    "../SD/M5NS.INI",
    "../../SD/M5NS.INI",
};

static size_t loadShippedIni(void) {
    FILE *f = NULL;
    for (unsigned i = 0; i < sizeof(candidatePaths) / sizeof(candidatePaths[0]); i++) {
        f = fopen(candidatePaths[i], "rb");
        if (f) break;
    }
    if (!f) {
        static char derived[1024];
        const char *self = __FILE__;
        const char *slash = strrchr(self, '/');
        if (slash) {
            size_t dirLen = (size_t)(slash - self);
            snprintf(derived, sizeof(derived), "%.*s/../../../SD/M5NS.INI", (int)dirLen, self);
            f = fopen(derived, "rb");
        }
    }
    TEST_ASSERT_NOT_NULL_MESSAGE(f, "shipped SD/M5NS.INI could not be opened");
    size_t n = fread(iniBuf, 1, sizeof(iniBuf) - 1, f);
    fclose(f);
    iniBuf[n] = '\0';
    return n;
}

static void parseShipped(void) {
    configDefaults(&cfg);
    size_t n = loadShippedIni();
    parseConfigBuffer(iniBuf, n, &cfg);
}

void test_sample_ini_parses_values(void) {
    configDefaults(&cfg);
    size_t n = loadShippedIni();
    int parsed = parseConfigBuffer(iniBuf, n, &cfg);
    TEST_ASSERT_GREATER_THAN_INT(10, parsed);
}

void test_sample_ini_has_no_unknown_keys(void) {
    parseShipped();
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, cfg.unknownKeys,
        "shipped INI contains keys the parser does not understand");
}

void test_sample_ini_declares_device_name(void) {
    parseShipped();
    TEST_ASSERT_EQUAL_STRING("t1display", cfg.deviceName);
}

void test_sample_ini_declares_nightscout_url(void) {
    parseShipped();
    TEST_ASSERT_TRUE(strlen(cfg.url) > 0);
}

void test_sample_ini_declares_wlan1(void) {
    parseShipped();
    TEST_ASSERT_TRUE(strlen(cfg.wlanssid[0]) > 0);
}

void test_sample_ini_thresholds_are_mmol(void) {
    parseShipped();
    TEST_ASSERT_FLOAT_WITHIN(11.5f, 13.5f, cfg.yellow_low);
    TEST_ASSERT_FLOAT_WITHIN(11.5f, 13.5f, cfg.yellow_high);
    TEST_ASSERT_FLOAT_WITHIN(11.5f, 13.5f, cfg.red_low);
    TEST_ASSERT_FLOAT_WITHIN(11.5f, 13.5f, cfg.red_high);
}

void test_sample_ini_alarm_thresholds_are_mmol(void) {
    parseShipped();
    TEST_ASSERT_FLOAT_WITHIN(11.5f, 13.5f, cfg.snd_alarm);
    TEST_ASSERT_FLOAT_WITHIN(11.5f, 13.5f, cfg.snd_warning);
    TEST_ASSERT_FLOAT_WITHIN(14.0f, 16.0f, cfg.snd_alarm_high);
    TEST_ASSERT_FLOAT_WITHIN(14.0f, 16.0f, cfg.snd_warning_high);
}

void test_sample_ini_threshold_ordering(void) {
    parseShipped();
    TEST_ASSERT_TRUE(cfg.red_low < cfg.yellow_low);
    TEST_ASSERT_TRUE(cfg.yellow_low < cfg.yellow_high);
    TEST_ASSERT_TRUE(cfg.yellow_high < cfg.red_high);
    TEST_ASSERT_TRUE(cfg.snd_alarm < cfg.snd_warning);
    TEST_ASSERT_TRUE(cfg.snd_warning_high < cfg.snd_alarm_high);
}

void test_sample_ini_normal_glucose_is_green(void) {
    parseShipped();
    int color = glucoseColor(6.0f, cfg.yellow_low, cfg.yellow_high,
                             cfg.red_low, cfg.red_high);
    TEST_ASSERT_EQUAL_INT_MESSAGE(GLUCOSE_COLOR_GREEN, color,
        "a normal 6.0 mmol/L reading must not render as out-of-range");
}

void test_sample_ini_normal_glucose_raises_no_alarm(void) {
    parseShipped();
    int level = alarmLevel(6.0f, cfg.snd_alarm, cfg.snd_warning,
                           cfg.snd_alarm_high, cfg.snd_warning_high,
                           0, (unsigned)cfg.snd_no_readings, false);
    TEST_ASSERT_EQUAL_INT_MESSAGE(ALARM_LEVEL_NORMAL, level,
        "a normal 6.0 mmol/L reading must not raise an alarm");
}

void test_sample_ini_hypo_raises_low_alarm(void) {
    parseShipped();
    int level = alarmLevel(2.5f, cfg.snd_alarm, cfg.snd_warning,
                           cfg.snd_alarm_high, cfg.snd_warning_high,
                           0, (unsigned)cfg.snd_no_readings, false);
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_LOW_ALARM, level);
}

void test_sample_ini_hyper_raises_high_alarm(void) {
    parseShipped();
    int level = alarmLevel(22.0f, cfg.snd_alarm, cfg.snd_warning,
                           cfg.snd_alarm_high, cfg.snd_warning_high,
                           0, (unsigned)cfg.snd_no_readings, false);
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_HIGH_ALARM, level);
}

void test_sample_ini_hypo_is_red(void) {
    parseShipped();
    int color = glucoseColor(3.0f, cfg.yellow_low, cfg.yellow_high,
                             cfg.red_low, cfg.red_high);
    TEST_ASSERT_EQUAL_INT(GLUCOSE_COLOR_RED, color);
}

void test_sample_ini_survives_roundtrip(void) {
    parseShipped();
    char out[4096];
    int written = serializeConfigINI(&cfg, out, sizeof(out));
    TEST_ASSERT_GREATER_THAN_INT(0, written);

    ParsedConfig again;
    configDefaults(&again);
    parseConfigBuffer(out, (size_t)written, &again);
    TEST_ASSERT_EQUAL_INT(0, again.unknownKeys);
    TEST_ASSERT_EQUAL_STRING(cfg.deviceName, again.deviceName);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, cfg.yellow_low, again.yellow_low);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, cfg.snd_alarm, again.snd_alarm);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_sample_ini_parses_values);
    RUN_TEST(test_sample_ini_has_no_unknown_keys);
    RUN_TEST(test_sample_ini_declares_device_name);
    RUN_TEST(test_sample_ini_declares_nightscout_url);
    RUN_TEST(test_sample_ini_declares_wlan1);
    RUN_TEST(test_sample_ini_thresholds_are_mmol);
    RUN_TEST(test_sample_ini_alarm_thresholds_are_mmol);
    RUN_TEST(test_sample_ini_threshold_ordering);
    RUN_TEST(test_sample_ini_normal_glucose_is_green);
    RUN_TEST(test_sample_ini_normal_glucose_raises_no_alarm);
    RUN_TEST(test_sample_ini_hypo_raises_low_alarm);
    RUN_TEST(test_sample_ini_hyper_raises_high_alarm);
    RUN_TEST(test_sample_ini_hypo_is_red);
    RUN_TEST(test_sample_ini_survives_roundtrip);
    return UNITY_END();
}
