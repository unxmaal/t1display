#include <unity.h>
#include "ns_pure_logic.h"

void setUp(void) {}
void tearDown(void) {}

void test_fresh_reading_is_zero_minutes(void) {
    TEST_ASSERT_EQUAL_INT(0, sensorAgeMinutes(1000, 1000));
}

void test_five_minutes(void) {
    TEST_ASSERT_EQUAL_INT(5, sensorAgeMinutes(1000 + 300, 1000));
}

void test_rounds_to_nearest_minute(void) {
    TEST_ASSERT_EQUAL_INT(1, sensorAgeMinutes(1000 + 31, 1000));
    TEST_ASSERT_EQUAL_INT(0, sensorAgeMinutes(1000 + 29, 1000));
}

void test_small_future_skew_is_zero(void) {
    TEST_ASSERT_EQUAL_INT(0, sensorAgeMinutes(1000, 1000 + SENSOR_FUTURE_TOLERANCE_SEC));
}

void test_far_future_timestamp_is_unknown(void) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(SENSOR_AGE_UNKNOWN,
        sensorAgeMinutes(1000, 1000 + SENSOR_FUTURE_TOLERANCE_SEC + 1),
        "a reading dated well in the future cannot be trusted as fresh");
    TEST_ASSERT_EQUAL_INT(SENSOR_AGE_UNKNOWN, sensorAgeMinutes(1000, 1000 + 3600));
}

void test_small_skew_does_not_raise_no_readings(void) {
    int age = sensorAgeMinutes(1000, 1000 + 60);
    TEST_ASSERT_EQUAL_INT(0, age);
    int level = alarmLevel(6.0f, 3.0f, 3.7f, 20.0f, 14.0f,
                           (unsigned)age, 20, false);
    TEST_ASSERT_EQUAL_INT_MESSAGE(ALARM_LEVEL_NORMAL, level,
        "a device clock behind the server must not fire a spurious no-readings alarm");
}

void test_unknown_now_is_unknown(void) {
    TEST_ASSERT_EQUAL_INT(SENSOR_AGE_UNKNOWN, sensorAgeMinutes(0, 1000));
}

void test_unknown_sensor_time_is_unknown(void) {
    TEST_ASSERT_EQUAL_INT(SENSOR_AGE_UNKNOWN, sensorAgeMinutes(1000, 0));
}

void test_unknown_age_raises_no_readings(void) {
    int level = alarmLevel(6.0f, 3.0f, 3.7f, 20.0f, 14.0f,
                           (unsigned)SENSOR_AGE_UNKNOWN, 20, false);
    TEST_ASSERT_EQUAL_INT_MESSAGE(ALARM_LEVEL_NO_READINGS, level,
        "an unknown age must fail loud, not silent");
}

void test_stale_hour(void) {
    TEST_ASSERT_EQUAL_INT(60, sensorAgeMinutes(1000 + 3600, 1000));
}

void test_very_old_reading_does_not_overflow(void) {
    int age = sensorAgeMinutes(2000000000L, 1000000000L);
    TEST_ASSERT_TRUE(age > 0);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_fresh_reading_is_zero_minutes);
    RUN_TEST(test_five_minutes);
    RUN_TEST(test_rounds_to_nearest_minute);
    RUN_TEST(test_small_future_skew_is_zero);
    RUN_TEST(test_far_future_timestamp_is_unknown);
    RUN_TEST(test_small_skew_does_not_raise_no_readings);
    RUN_TEST(test_unknown_now_is_unknown);
    RUN_TEST(test_unknown_sensor_time_is_unknown);
    RUN_TEST(test_unknown_age_raises_no_readings);
    RUN_TEST(test_stale_hour);
    RUN_TEST(test_very_old_reading_does_not_overflow);
    return UNITY_END();
}
