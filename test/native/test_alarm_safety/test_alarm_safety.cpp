#include <unity.h>
#include <string.h>
#include <math.h>
#include "ns_pure_logic.h"
#include "ns_config_parse.h"

void setUp(void) {}
void tearDown(void) {}

static const float A_LO = 3.0f, W_LO = 3.7f, A_HI = 20.0f, W_HI = 14.0f;
static const unsigned NO_READ = 20;

static char buf[2048];

static size_t loadBuf(const char *src) {
    size_t len = strlen(src);
    memcpy(buf, src, len + 1);
    return len;
}

static int levelFor(float sgv, unsigned age) {
    return alarmLevel(sgv, A_LO, W_LO, A_HI, W_HI, age, NO_READ, false);
}

void test_fresh_zero_sgv_is_not_normal(void) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(ALARM_LEVEL_NO_READINGS, levelFor(0.0f, 0),
        "a fresh sgv of 0 must not be reported as a normal reading");
}

void test_fresh_dexcom_sentinel_one_is_not_normal(void) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(ALARM_LEVEL_NO_READINGS, levelFor(1.0f / MGDL_PER_MMOL, 0),
        "a fresh Dexcom sentinel of 1 mg/dL must not be reported as normal");
}

void test_fresh_negative_sgv_is_not_normal(void) {
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_NO_READINGS, levelFor(-5.0f, 0));
}

void test_stale_zero_sgv_still_no_readings(void) {
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_NO_READINGS, levelFor(0.0f, 25));
}

void test_real_hypo_still_alarms(void) {
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_LOW_ALARM, levelFor(2.5f, 0));
}

void test_dexcom_error_codes_are_no_readings(void) {
    const float codes[] = {2, 3, 5, 9, 10, 12, 38};
    for (size_t i = 0; i < sizeof(codes) / sizeof(codes[0]); i++) {
        TEST_ASSERT_EQUAL_INT_MESSAGE(ALARM_LEVEL_NO_READINGS,
            levelFor(codes[i] / MGDL_PER_MMOL, 0),
            "an sgv below 39 mg/dL is a sensor error code, not a hypo");
    }
}

void test_dexcom_low_reading_of_39_alarms(void) {
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_LOW_ALARM, levelFor(39.0f / MGDL_PER_MMOL, 0));
}

void test_sensor_error_predicate(void) {
    TEST_ASSERT_TRUE(sgvIsSensorError(0.0f));
    TEST_ASSERT_TRUE(sgvIsSensorError(38.0f / MGDL_PER_MMOL));
    TEST_ASSERT_TRUE(sgvIsSensorError(NAN));
    TEST_ASSERT_FALSE(sgvIsSensorError(39.0f / MGDL_PER_MMOL));
    TEST_ASSERT_FALSE(sgvIsSensorError(6.0f));
}

void test_normal_reading_unaffected(void) {
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_NORMAL, levelFor(6.0f, 0));
}

void test_high_alarm_unaffected(void) {
    TEST_ASSERT_EQUAL_INT(ALARM_LEVEL_HIGH_ALARM, levelFor(22.0f, 0));
}

void test_nan_threshold_rejected_at_parse(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t n = loadBuf("[config]\nsnd_alarm = nan\n");
    parseConfigBuffer(buf, n, &cfg);
    TEST_ASSERT_TRUE_MESSAGE(isfinite(cfg.snd_alarm),
        "a non-finite threshold must never reach the alarm path");
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, cfg.snd_alarm);
}

void test_inf_threshold_rejected_at_parse(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t n = loadBuf("[config]\nred_low = inf\nyellow_low = -inf\n");
    parseConfigBuffer(buf, n, &cfg);
    TEST_ASSERT_TRUE(isfinite(cfg.red_low));
    TEST_ASSERT_TRUE(isfinite(cfg.yellow_low));
}

void test_nan_threshold_cannot_silence_hypo(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t n = loadBuf("[config]\nsnd_alarm = nan\nsnd_warning = nan\n");
    parseConfigBuffer(buf, n, &cfg);
    int level = alarmLevel(2.5f, cfg.snd_alarm, cfg.snd_warning,
                           cfg.snd_alarm_high, cfg.snd_warning_high,
                           0, (unsigned)cfg.snd_no_readings, false);
    TEST_ASSERT_EQUAL_INT_MESSAGE(ALARM_LEVEL_LOW_ALARM, level,
        "a severe hypo must alarm even after a malformed threshold");
}

void test_empty_threshold_value_falls_back_to_default(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t n = loadBuf("[config]\nsnd_alarm = \n");
    parseConfigBuffer(buf, n, &cfg);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, cfg.snd_alarm);
}

void test_out_of_range_threshold_rejected(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t n = loadBuf("[config]\nsnd_alarm = 70\nred_low = 80\n");
    parseConfigBuffer(buf, n, &cfg);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, cfg.snd_alarm);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.9f, cfg.red_low);
}

void test_integer_overflow_rejected(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t n = loadBuf("[config]\ntime_zone = 99999999999999999999\n");
    parseConfigBuffer(buf, n, &cfg);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3600, cfg.timeZone,
        "an unparsable time_zone must fall back to the default, not to atoi garbage");
}

void test_restart_at_logged_errors_clamped(void) {
    ParsedConfig cfg;
    configDefaults(&cfg);
    size_t n = loadBuf("[config]\nrestart_at_logged_errors = -5\n");
    parseConfigBuffer(buf, n, &cfg);
    TEST_ASSERT_TRUE(cfg.restart_at_logged_errors >= 0);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_fresh_zero_sgv_is_not_normal);
    RUN_TEST(test_fresh_dexcom_sentinel_one_is_not_normal);
    RUN_TEST(test_fresh_negative_sgv_is_not_normal);
    RUN_TEST(test_stale_zero_sgv_still_no_readings);
    RUN_TEST(test_real_hypo_still_alarms);
    RUN_TEST(test_dexcom_error_codes_are_no_readings);
    RUN_TEST(test_dexcom_low_reading_of_39_alarms);
    RUN_TEST(test_sensor_error_predicate);
    RUN_TEST(test_normal_reading_unaffected);
    RUN_TEST(test_high_alarm_unaffected);
    RUN_TEST(test_nan_threshold_rejected_at_parse);
    RUN_TEST(test_inf_threshold_rejected_at_parse);
    RUN_TEST(test_nan_threshold_cannot_silence_hypo);
    RUN_TEST(test_empty_threshold_value_falls_back_to_default);
    RUN_TEST(test_out_of_range_threshold_rejected);
    RUN_TEST(test_integer_overflow_rejected);
    RUN_TEST(test_restart_at_logged_errors_clamped);
    return UNITY_END();
}
