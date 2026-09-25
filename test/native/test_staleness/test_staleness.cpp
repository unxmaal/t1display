#include <unity.h>
#include <string.h>
#include "ns_display_model.h"
#include "ns_pure_logic.h"

void setUp(void) {}
void tearDown(void) {}

static GlucosePageModel m;

static void build(long now_sec, long sensor_sec) {
    buildGlucoseModel(&m,
        6.0f, 108.0f, 0,
        "Flat", 0,
        "+0.1",
        12, 30,
        now_sec, sensor_sec,
        4.5f, 9.0f, 3.9f, 11.0f,
        ALARM_LEVEL_NORMAL, 0,
        80, 0, 0);
}

void test_fresh_reading_is_fresh(void) {
    build(10000, 10000);
    TEST_ASSERT_EQUAL_INT(STALENESS_FRESH, m.staleness);
}

void test_fresh_reading_keeps_range_colour(void) {
    build(10000, 10000);
    TEST_ASSERT_EQUAL_INT(COLOR_GREEN, m.glucose_color);
    TEST_ASSERT_EQUAL_STRING("6.0", m.glucose_str);
}

void test_stale_reading_is_marked_stale(void) {
    build(10000 + 15 * 60, 10000);
    TEST_ASSERT_EQUAL_INT(STALENESS_STALE, m.staleness);
}

void test_stale_reading_loses_range_colour(void) {
    build(10000 + 15 * 60, 10000);
    TEST_ASSERT_EQUAL_INT_MESSAGE(COLOR_LIGHTGREY, m.glucose_color,
        "a stale value must not keep signalling that it is in range");
}

void test_stale_reading_is_struck_through(void) {
    build(10000 + 15 * 60, 10000);
    TEST_ASSERT_TRUE(m.strike_glucose);
}

void test_no_data_replaces_the_number(void) {
    build(10000 + 40 * 60, 10000);
    TEST_ASSERT_EQUAL_INT(STALENESS_NO_DATA, m.staleness);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("--.-", m.glucose_str,
        "past the no-data threshold the last value must not be displayed as current");
}

void test_no_data_shows_banner_with_age(void) {
    build(10000 + 40 * 60, 10000);
    TEST_ASSERT_TRUE(m.show_banner);
    TEST_ASSERT_NOT_NULL(strstr(m.banner_str, "NO DATA"));
    TEST_ASSERT_NOT_NULL(strstr(m.banner_str, "40"));
}

void test_no_data_keeps_last_value_as_secondary(void) {
    build(10000 + 40 * 60, 10000);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(m.last_seen_str, "6.0"),
        "the last known value stays visible but clearly labelled as past");
}

void test_unknown_clock_is_no_data(void) {
    build(0, 10000);
    TEST_ASSERT_EQUAL_INT(STALENESS_NO_DATA, m.staleness);
}

void test_unknown_sensor_time_is_no_data(void) {
    build(10000, 0);
    TEST_ASSERT_EQUAL_INT(STALENESS_NO_DATA, m.staleness);
}

void test_slightly_future_reading_is_fresh(void) {
    build(10000, 10000 + 60);
    TEST_ASSERT_EQUAL_INT(STALENESS_FRESH, m.staleness);
}

void test_far_future_reading_is_no_data(void) {
    build(10000, 10000 + 3600);
    TEST_ASSERT_EQUAL_INT_MESSAGE(STALENESS_NO_DATA, m.staleness,
        "a reading from a fast uploader clock must not keep a dead feed looking live");
}

void test_normal_upload_lag_is_fresh(void) {
    for (int min = 5; min <= 10; min++) {
        build(10000 + min * 60, 10000);
        TEST_ASSERT_EQUAL_INT_MESSAGE(STALENESS_FRESH, m.staleness,
            "a reading waiting on the next 5-minute upload is not stale");
    }
}

void test_one_missed_reading_is_stale(void) {
    build(10000 + 11 * 60, 10000);
    TEST_ASSERT_EQUAL_INT(STALENESS_STALE, m.staleness);
}

static void buildSgv(float mgdl) {
    buildGlucoseModel(&m,
        mgdl / MGDL_PER_MMOL, mgdl, 0,
        "Flat", 0,
        "",
        12, 30,
        10000, 10000,
        4.5f, 9.0f, 3.9f, 11.0f,
        ALARM_LEVEL_NO_READINGS, 0,
        80, 0, 0);
}

void test_sensor_error_code_is_not_displayed_as_a_value(void) {
    buildSgv(5.0f);
    TEST_ASSERT_EQUAL_STRING("--.-", m.glucose_str);
    TEST_ASSERT_EQUAL_INT(COLOR_LIGHTGREY, m.glucose_color);
    TEST_ASSERT_EQUAL_INT(180, m.arrow_angle);
    TEST_ASSERT_TRUE(m.show_banner);
    TEST_ASSERT_EQUAL_STRING("SENSOR ERROR", m.banner_str);
    TEST_ASSERT_EQUAL_STRING("", m.last_seen_str);
}

void test_stale_sensor_error_code_is_not_displayed_as_a_value(void) {
    buildGlucoseModel(&m,
        5.0f / MGDL_PER_MMOL, 5.0f, 0,
        "Flat", 0,
        "",
        12, 30,
        10000 + 15 * 60, 10000,
        4.5f, 9.0f, 3.9f, 11.0f,
        ALARM_LEVEL_NO_READINGS, 0,
        80, 0, 0);
    TEST_ASSERT_EQUAL_STRING("--.-", m.glucose_str);
    TEST_ASSERT_EQUAL_STRING("SENSOR ERROR", m.banner_str);
}

void test_dexcom_low_of_39_is_displayed(void) {
    buildSgv(39.0f);
    TEST_ASSERT_FALSE(m.show_banner);
    TEST_ASSERT_EQUAL_INT(COLOR_RED, m.glucose_color);
}

void test_boundary_at_stale_threshold(void) {
    build(10000 + SENSOR_AGE_STALE_MIN * 60, 10000);
    TEST_ASSERT_EQUAL_INT(STALENESS_FRESH, m.staleness);
    build(10000 + (SENSOR_AGE_STALE_MIN + 1) * 60, 10000);
    TEST_ASSERT_EQUAL_INT(STALENESS_STALE, m.staleness);
}

void test_age_text_still_shown_when_stale(void) {
    build(10000 + 15 * 60, 10000);
    TEST_ASSERT_TRUE(m.show_age);
    TEST_ASSERT_NOT_NULL(strstr(m.age_str, "15"));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_fresh_reading_is_fresh);
    RUN_TEST(test_fresh_reading_keeps_range_colour);
    RUN_TEST(test_stale_reading_is_marked_stale);
    RUN_TEST(test_stale_reading_loses_range_colour);
    RUN_TEST(test_stale_reading_is_struck_through);
    RUN_TEST(test_no_data_replaces_the_number);
    RUN_TEST(test_no_data_shows_banner_with_age);
    RUN_TEST(test_no_data_keeps_last_value_as_secondary);
    RUN_TEST(test_unknown_clock_is_no_data);
    RUN_TEST(test_unknown_sensor_time_is_no_data);
    RUN_TEST(test_slightly_future_reading_is_fresh);
    RUN_TEST(test_far_future_reading_is_no_data);
    RUN_TEST(test_normal_upload_lag_is_fresh);
    RUN_TEST(test_one_missed_reading_is_stale);
    RUN_TEST(test_sensor_error_code_is_not_displayed_as_a_value);
    RUN_TEST(test_stale_sensor_error_code_is_not_displayed_as_a_value);
    RUN_TEST(test_dexcom_low_of_39_is_displayed);
    RUN_TEST(test_boundary_at_stale_threshold);
    RUN_TEST(test_age_text_still_shown_when_stale);
    return UNITY_END();
}
